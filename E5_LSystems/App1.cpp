// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"
#include <stack>

App1::App1():
	lSystem("SFFFA")
{
	m_Line = nullptr;
	m_InstanceShader = nullptr;
	shader = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	textureMgr->loadTexture(L"grass", L"res/grass.png");
	textureMgr->loadTexture(L"brick", L"res/Brick.png");

	// Create Mesh object and shader object
	m_Line = new LineMesh(renderer->getDevice(), renderer->getDeviceContext());
	shader = new LightShader(renderer->getDevice(), hwnd);

	m_InstancedCube = new InstancedCubeMesh(renderer->getDevice(), renderer->getDeviceContext(), 1);
	m_InstanceShader = new InstanceShader(renderer->getDevice(), hwnd);

	light = new Light;
	light->setAmbientColour( 0.25f, 0.25f, 0.25f, 1.0f );
	light->setDiffuseColour(0.75f, 0.75f, 0.75f, 1.0f);
	light->setDirection(1.0f,-0.0f, 0.0f);

	camera->setPosition(0.0f, 2.0f, -15.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	
	//Build the LSystem
	lSystem.AddRule('A', "SFFFA");
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

	m_InstanceShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), light);
	m_InstancedCube->sendDataInstanced(renderer->getDeviceContext());
	m_InstanceShader->renderInstanced(renderer->getDeviceContext(), m_InstancedCube->getIndexCount(), m_InstancedCube->GetInstanceCount());
	
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

	ImGui::Text(lSystem.GetCurrentSystem().c_str());
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

//Render a bunch of instanced cubes
void App1::BuildCubeInstances() {

	const int width = 64;
	const int maxCubes = width * width * width;

	XMFLOAT3* pos = new XMFLOAT3[maxCubes];

	int instanceCount = 0;
	//Create two crossing sine waves and only draw the cubes that are under the "height" value
	for (int i = 0; i < maxCubes; i++) {
		float y1 = sin((float)(i % width) / 8.0f);
		y1 += 1.0f;
		y1 *= 16.f;

		float y2 = sin((float)(i / (width * width)) / 4.0f);
		y2 += 1.0f;
		y2 *= 16.f;

		if ((i / width) % width < y1 && (i / width) % width < y2) {
			pos[instanceCount] = XMFLOAT3(2.0f * (i % width), 2.0f * ((i / width) % width), 2.0f * (i / (width * width)));
			instanceCount++;
		}
	}

	m_InstancedCube->initBuffers(renderer->getDevice(), pos, instanceCount);

	delete[] pos;
	pos = 0;
}

void App1::BuildLine()
{
	const int width = 64;
	const int maxCubes = width * width * width * width;
	int instanceCount = 0;

	XMFLOAT3* cubePos = new XMFLOAT3[maxCubes];

	std::string line;
	int val;

	//set the initial state based on the input from the user
	//the user can enter how long they wish the initial line to be
	//this will soon be changed to the number of rooms in the dungeon
	//TODO: change this to impact number of rooms in the dungeon
	line += 'S';

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

	p = XMFLOAT3(1, 0, 0);
		XMVECTOR fwdX = XMLoadFloat3(&p);	//X Rotation axis

	p = XMFLOAT3(0, 1, 0);
		XMVECTOR fwdY = XMLoadFloat3(&p);	//Y Rotation axis

	p = XMFLOAT3(0, 0, 1);
		XMVECTOR fwdZ = XMLoadFloat3(&p);	//Y Rotation axis
											
											//A matrix that stores the current rotation state
	XMMATRIX rotation = XMMatrixRotationAxis(fwdZ, 0.0f);

	//Start and end of each line segment
	XMFLOAT3 start, end;

	rotationStack.push(rotation);
	savePoint.push(pos);

	//Go through the L-System string and perform some action depending on which
	//character is encountered
	for (int i = 0; i < systemString.length(); i++) {

		switch (systemString[i]) {
		case 'F':
			//Place a cube at the current position then increase the position in the direction the
			//current branch is pointing
			XMFLOAT3 currentPos;
			XMStoreFloat3(&currentPos, pos);

			dir = XMVector3Transform(fwdZ, rotation);
			cubePos[instanceCount] = currentPos;

			//XMStoreFloat3(&start, pos);		
			pos += dir * 2.f;				//Multiply direction by 2 as cubes are 2 units wide
			//XMStoreFloat3(&end, pos);			
			//m_Line->AddLine(start, end);	

			instanceCount++;
			break;

		case '[':
			//push the current position and rotation onto the stack so we can return to
			//it later
			savePoint.push(pos);
			rotationStack.push(rotation);
			break;

		case ']':
			//return the position and rotation to the values that were last pushed onto the stack
			pos = savePoint.top();
			savePoint.pop();

			rotation = rotationStack.top();
			rotationStack.pop();

			//Reset the direction back to what it would have been here, based on the rotation
			dir = XMVector3Transform(fwdZ, rotation);
			break;

		case '&':
			//pick a random number between 10 and 25 then rotate around the x-axis 
			//by the chosen number of degrees 
			val = ((rand() % 15) + 10);

			rotation = XMMatrixRotationAxis(fwdX, AI_DEG_TO_RAD(val)) * rotation; //This one needs to be rotated in this order, for some reason...
			break;

		case '/':
			//pick a random number between 60 and 120 then rotate around the current direction 
			//by the chosen number of degrees
			val = ((rand() % 60) + 60);

			rotation *= XMMatrixRotationAxis(dir, AI_DEG_TO_RAD(val));
			break;

		case '\\':
			//pick a random number between -120 and -60 then rotate around the current direction 
			//by the chosen number of degrees
			val = -((rand() % 60) + 60);

			rotation *= XMMatrixRotationAxis(dir, AI_DEG_TO_RAD(val));
			break;

		case 'S':
			XMVECTOR corner;
			XMVECTOR corners[4];
			corner = corners[0] = XMVector3Transform(pos, XMMatrixTranslation(-20, 0, 0));  //Bottom left

			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslation(40, 0, 0)); //Bottom right

			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslation(0, 0, 30)); //Top right

			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslation(-40, 0, 0)); //Top left

			//Set the pos vector back to the centre of the room on the back wall
			//This will eventually be replaced by picking a random position on a wall
			XMVECTOR temp = corners[3] - corners[2];
			temp = XMVector3Length(temp);
			XMStoreFloat3(&start, temp);

			pos = XMVector3Transform(corners[2], XMMatrixTranslation(-start.x / 2, 0, 0));

			//Build a room using the corners provided
			BuildRoom(corners, cubePos, instanceCount, 10.f);
			break;

		}
	}

	m_InstancedCube->initBuffers(renderer->getDevice(), cubePos, instanceCount);

	delete[] cubePos;
	cubePos = 0;
}

//Corners must be in order bottom left -> bottom right -> top right -> top left
void App1::BuildRoom(XMVECTOR* corners, XMFLOAT3* vectorPos, int& cubeInstance, float height)
{
	XMFLOAT3 lengths[4];
	XMVECTOR walls[4];

	walls[0] = XMVECTOR(corners[1] - corners[0]);
	walls[1] = XMVECTOR(corners[2] - corners[1]);
	walls[2] = XMVECTOR(corners[2] - corners[3]);
	walls[3] = XMVECTOR(corners[3] - corners[0]);

	XMStoreFloat3(&lengths[0], XMVector3Length(walls[0]));
	XMStoreFloat3(&lengths[1], XMVector3Length(walls[1]));
	XMStoreFloat3(&lengths[2], XMVector3Length(walls[2]));
	XMStoreFloat3(&lengths[3], XMVector3Length(walls[3]));

	float longestVector = lengths[0].x;

	for (int i = 1; i < 4; i++)
	{
		if (longestVector < lengths[i].x)
		{
			longestVector = lengths[i].x;
		}
	}

	int longest;
	longest = ceil(longestVector);

	XMVECTOR pos[4];
	XMVECTOR target[4];
	XMVECTOR distanceLeft;

	XMFLOAT3 roomCornerPos;
	pos[0] = corners[0];
	pos[1] = corners[1];
	pos[2] = corners[3];
	pos[3] = corners[0];

	for (int i = 0; i < 4; i++)
	{
		target[i] = walls[i] + pos[i];
	}

	for (int i = 0; i < longest / 2; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			distanceLeft = target[j] - pos[j];
			distanceLeft = XMVector3Normalize(distanceLeft);

			//Pretty much this whole if statement is super janky, please fix this if
			//you have time that said.... IT WORKS!
			if (XMVector3GreaterOrEqual(distanceLeft, XMVECTOR{ 0,0,0 }))
			{
				//Build the floor
				if (j == 3)
				{
					XMVECTOR floorBuilder = pos[3];
					XMFLOAT3 floorPos;

					for (int k = 0; k < lengths[0].x / 2; k++)
					{
						XMStoreFloat3(&floorPos, floorBuilder);
						vectorPos[cubeInstance] = floorPos;
						cubeInstance++;

						floorBuilder += XMVECTOR{ 2,0,0 };
					}
				}

				//Build the walls
				for (int k = 0; k < (int)height / 2; k++)
				{
					XMStoreFloat3(&roomCornerPos, pos[j]);
					vectorPos[cubeInstance] = roomCornerPos;
					cubeInstance++;

					pos[j] += XMVECTOR{ 0,2,0 };
				}

				//Build the roof
				if (j == 3)
				{
					XMVECTOR roofBuilder = pos[3];
					XMFLOAT3 roofPos;

					for (int k = 0; k < lengths[0].x / 2; k++)
					{
						XMStoreFloat3(&roofPos, roofBuilder);
						vectorPos[cubeInstance] = roofPos;
						cubeInstance++;

						roofBuilder += XMVECTOR{ 2,0,0 };
					}
				}

				pos[j] -= XMVECTOR{ 0,height,0 };

				//Builds the floor out, kinda janky with some magic numbers
				//might be able to fix this later
				

				pos[j] += (XMVector3Normalize(walls[j]) * 2);
			}
		}
	}
}
