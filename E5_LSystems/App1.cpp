// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"
#include <stack>

App1::App1():
	lSystem("FA")
{
	m_Line = nullptr;
	shader = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	textureMgr->loadTexture(L"grass", L"res/grass.png");

	// Create Mesh object and shader object
	m_Line = new LineMesh(renderer->getDevice(), renderer->getDeviceContext());
	shader = new LightShader(renderer->getDevice(), hwnd);

	light = new Light;
	light->setAmbientColour( 0.25f, 0.25f, 0.25f, 1.0f );
	light->setDiffuseColour(0.75f, 0.75f, 0.75f, 1.0f);
	light->setDirection(1.0f,-0.0f, 0.0f);

	camera->setPosition(0.0f, 2.0f, -15.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	
	//Build the LSystem
	lSystem.AddRule('A', "[&FA][&/FA][&\\FA]");
	lSystem.Run(8);

	//Build the lines to be rendered
	BuildLine();
}


App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
	if (m_Line)
	{
		delete m_Line;
		m_Line = 0;
	}
	if (shader)
	{
		delete shader;
		shader = 0;
	}
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}
	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::render()
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix;

	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);

	// Generate the view matrix based on the camera's position.
	camera->update();

	// Get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	worldMatrix = renderer->getWorldMatrix();
	viewMatrix = camera->getViewMatrix();
	projectionMatrix = renderer->getProjectionMatrix();

	//All our lines are drawn with the same shader parameters, so we can call this once
	shader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"grass"), light);
	//Send each line segment seperately for rendering
	for (int i = 0; i < m_Line->GetLineCount(); i++) {
		m_Line->sendData(renderer->getDeviceContext(), i);
		shader->render(renderer->getDeviceContext(), m_Line->getIndexCount());
	}
	
	// Render GUI
	gui();

	// Swap the buffers
	renderer->endScene();

	return true;
}

void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Text( "Camera Pos: (%.2f, %.2f, %.2f)", camera->getPosition().x, camera->getPosition().y, camera->getPosition().z );
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	//ImGui::Text(lSystem.GetCurrentSystem().c_str());
	ImGui::InputInt("Length of starting line", &startingLine);
	ImGui::InputInt("Number of LSystem Iterations", &numIterate);

	if (ImGui::Button("Build Tree"))
	{
		BuildLine();
	}

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void App1::BuildLine()
{
	std::string line;

	//set the initial state based on the input from the user
	//the user can enter how long they wish the initial line to be
	//this will soon be changed to the number of rooms in the dungeon
	//TODO: change this to impact number of rooms in the dungeon
	for (int i = 0; i < startingLine; i++)
	{
		line += 'F';
	}

	line += 'A';

	lSystem.ChangeAxiom(line);
	lSystem.Run(numIterate);

	std::stack<XMVECTOR> savePoint;
	std::stack<XMMATRIX> rotationStack;

	//Clear any lines we might already have
	m_Line->Clear();

	//Get the current L-System string
	std::string systemString = lSystem.GetCurrentSystem();

	//Initialise some variables
	XMFLOAT3 p(0, 0, 0);
		XMVECTOR pos = XMLoadFloat3(&p);	//Current position

	p = XMFLOAT3(0, 1, 0);
		XMVECTOR dir = XMLoadFloat3(&p);	//Current direction

	p = XMFLOAT3(0, 1, 0);
		XMVECTOR fwdY = XMLoadFloat3(&p);	//Y Rotation axis

	p = XMFLOAT3(1, 0, 0);
		XMVECTOR fwdX = XMLoadFloat3(&p);	//X Rotation axis

	//A matrix that stores the current rotation state
	XMMATRIX rotation = XMMatrixRotationAxis(fwdY, 0.0f);

	//Start and end of each line segment
	XMFLOAT3 start, end;


	rotationStack.push(rotation);
	savePoint.push(pos);

	//Go through the L-System string
	for (int i = 0; i < systemString.length(); i++) {

		switch (systemString[i]) {
		case 'F':
			dir = XMVector3Transform(fwdY, rotation);
			XMStoreFloat3(&start, pos);			//Store the start position
			pos += dir;							//Move the position marker
			XMStoreFloat3(&end, pos);			//Store the end position
			m_Line->AddLine(start, end);		//Create the line
			break;

		case '[':
			savePoint.push(pos);
			rotationStack.push(rotation);
			break;

		case ']':
			pos = savePoint.top();
			savePoint.pop();

			rotation = rotationStack.top();
			rotationStack.pop();

			//Reset the direction back to what it would have been here, based on the rotation
			dir = XMVector3Transform(fwdY, rotation);
			break;

		case '&':
			rotation = XMMatrixRotationAxis(fwdX, AI_DEG_TO_RAD((rand() % 15) + 10)) * rotation; //This one needs to be rotated in this order, for some reason...
			break;

		case '/':
			rotation *= XMMatrixRotationAxis(dir, AI_DEG_TO_RAD((rand() % 30) + 90));
			break;

		case '\\':
			rotation *= XMMatrixRotationAxis(dir, AI_DEG_TO_RAD(-(rand() % 30) + 90));
			break;

		}
	}
}

