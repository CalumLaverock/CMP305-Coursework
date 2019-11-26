// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"
#include <stack>
#include <ctime>

App1::App1():
	lSystem("S")
{
	m_InstanceShader = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	srand(time(NULL));

	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	textureMgr->loadTexture(L"wood", L"res/ground_01.png");
	textureMgr->loadTexture(L"brick", L"res/ground_06.png");

	m_InstancedCube = new InstancedCubeMesh(renderer->getDevice(), renderer->getDeviceContext(), 1);
	m_InstancedCubeFloor = new InstancedCubeMesh(renderer->getDevice(), renderer->getDeviceContext(), 1);
	m_InstanceShader = new InstanceShader(renderer->getDevice(), hwnd);

	light = new Light;
	light->setAmbientColour( 0.25f, 0.25f, 0.25f, 1.0f );
	light->setDiffuseColour(0.75f, 0.75f, 0.75f, 1.0f);
	light->setDirection(1.0f,-0.0f, 0.0f);

	camera->setPosition(0.0f, 6.0f, 5.0f);
	camera->setRotation(0.0f, 0.0f, 0.0f);
	
	//Build the LSystem
	lSystem.AddRule('A', "{/F}R{&F}A");
	lSystem.AddRule('C', "A{&F}R{&F}A");
	lSystem.Run(8);

	//Build the lines to be rendered
	BuildLine();
}


App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
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

	m_InstanceShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), light);
	m_InstancedCube->sendDataInstanced(renderer->getDeviceContext());
	m_InstanceShader->renderInstanced(renderer->getDeviceContext(), m_InstancedCube->getIndexCount(), m_InstancedCube->GetInstanceCount());

	m_InstanceShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"wood"), light);
	m_InstancedCubeFloor->sendDataInstanced(renderer->getDeviceContext());
	m_InstanceShader->renderInstanced(renderer->getDeviceContext(), m_InstancedCubeFloor->getIndexCount(), m_InstancedCubeFloor->GetInstanceCount());
	
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
	using namespace DirectX;

	const int width = 64;
	const int maxCubes = width * width * width * width;

	int instanceCount = 0;
	int floorInstanceCount = 0;

	XMFLOAT3* cubePos = new XMFLOAT3[maxCubes];
	XMFLOAT3* floorCubes = new XMFLOAT3[maxCubes];

	std::string line;
	int val;

	//The user can select how many rooms they want to have in the dungeon
	line += "S{&F}R{&F}A";

	/*for (int i = 0; i < startingLine; i++)
	{
		line += "ARACA";
	}*/

	line += "E";

	lSystem.ChangeAxiom(line);
	lSystem.Run(numIterate);

	std::stack<XMVECTOR> savePoint;
	std::stack<XMMATRIX> rotationStack;

	//Get the current L-System string
	std::string systemString = lSystem.GetCurrentSystem();

	//Initialise some variables
	XMFLOAT3 p(0, 0, 0);
		XMVECTOR pos = XMLoadFloat3(&p);	//Current position

	p = XMFLOAT3(0, 0, 1);
		XMVECTOR dir = XMLoadFloat3(&p);	//Current direction

	p = XMFLOAT3(0, 1, 0);
		XMVECTOR fwdY = XMLoadFloat3(&p);	//Y Rotation axis

	p = XMFLOAT3(0, 0, 1);
		fwd = XMLoadFloat3(&p);	//Z Rotation axis
		up = XMVECTOR{ 0,1,0 };
		right = XMVector3Cross(up, fwd);

	//A matrix that stores the current rotation state
	XMMATRIX rotation = XMMatrixRotationAxis(fwd, 0.0f);

	rotationStack.push(rotation);
	savePoint.push(pos);

	//Go through the L-System string and perform some action depending on which
	//character is encountered
	for (int i = 0; i < systemString.length(); i++) {
		currentSystemChar = systemString[i];

		switch (systemString[i]) {
		case 'F':
			dir = XMVector3Transform(fwd, rotation);

			//Build a tunnel when F is found
			BuildTunnel(dir, cubePos, floorCubes, instanceCount, floorInstanceCount);
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
			dir = XMVector3Transform(fwd, rotation);
			break;

		case '{':
			rotationStack.push(rotation);
			break;

		case '}':
			rotation = rotationStack.top();
			rotationStack.pop();

			//Reset the direction back to what it would have been here, based on the rotation
			dir = XMVector3Transform(fwd, rotation);
			break;

		case '&':
			//pick a random number between 10 and 25 then rotate around the x-axis 
			//by the chosen number of degrees 
			val = ((rand() % 50) - 25);

			rotation = XMMatrixRotationAxis(fwdY, AI_DEG_TO_RAD(val)) * rotation; //This one needs to be rotated in this order, for some reason...
			break;

		case '/':
			//pick a random number between 60 and 120 then rotate around the current direction 
			//by the chosen number of degrees
			val = ((rand() % 50) - 25);

			rotation *= XMMatrixRotationAxis(endRight, AI_DEG_TO_RAD(val));
			break;

		case '\\':
			//pick a random number between -120 and -60 then rotate around the current direction 
			//by the chosen number of degrees
			val = -((rand() % 60) + 60);

			rotation *= XMMatrixRotationAxis(endRight, AI_DEG_TO_RAD(val));
			break;

		case 'S':
		{
			XMVECTOR corner;
			XMVECTOR corners[4];
			corners[0] = corner = XMVector3Transform(pos, XMMatrixTranslation(-20, 0, 0));  //Bottom left
			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslation(40, 0, 0)); //Bottom right
			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslation(0, 0, 30)); //Top right
			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslation(-40, 0, 0)); //Top left

			//Set the pos vector back to the centre of the room on the back wall
			//This will eventually be replaced by picking a random position on a wall
			XMFLOAT3 endFloat;
			XMVECTOR endPosition;

			//Pick a random corner for the exit wall
			int endCornerOne = (rand() % 3) + 1;
			int endCornerTwo;

			//Choose the second corner based on the first random corner such that the exit wall
			//is never the same wall as the entrance wall
			//i.e. if endCornerOne == 1, endCornerTwo = 2 else if endCornerOne == 2, endCornerTwo = 3 else endCornerOne == 3, endCornerTwo = 0 
			//Also set forward and right vectors based on which wall was selected for the exit
			switch (endCornerOne)
			{
			case 1:
				endFwd = right;
				endCornerTwo = 2;
				break;
			case 2:
				endFwd = fwd;
				endCornerTwo = 3;
				break;
			case 3:
				endFwd = -right;
				endCornerTwo = 0;
				break;
			}

			endRight = XMVector3Cross(up, endFwd);

			XMVECTOR endVec = corners[endCornerOne] - corners[endCornerTwo];
			XMVECTOR wallDirection = XMVector3Normalize(endVec);
			endVec = XMVector3Length(endVec);
			XMStoreFloat3(&endFloat, endVec);
			//make the exit point a random value on the selected wall with a gap of 2 cubes (each of width 2, hence the range being 6 -> wall length - 6) 
			//on either side of the wall that is not allowed to be the exit point
			int exitPoint = (rand() % ((int)endFloat.x - 12)) + 6;
			exitPoint = (exitPoint % 2 == 1 ? exitPoint + 1 : exitPoint);

			//Set the end point on the chosen wall by moving from one of the corners along the wallDirection vector
			//by the randomly chosen exitPoint value
			startPosition = pos;
			pos = XMVector3Transform(corners[endCornerTwo], XMMatrixTranslationFromVector(wallDirection * exitPoint));
			endPosition = pos;

			//Randomly select a height between 8 and 18, the value needs to be even as the cubes are
			//2 units on each side
			int height = (rand() % 10) + 8;
			height = (height % 2 == 1 ? height + 1 : height);

			//Build a room using the corners provided
			BuildRoom(corners, cubePos, floorCubes, endPosition, instanceCount, floorInstanceCount, height);

			fwd = endFwd;
			right = endRight;
			//Set the next start position to the current end position so that the tunnel knows where to draw from
			startPosition = endPosition;
			break;
		}

		case 'R':
		{
			XMVECTOR corner;
			XMVECTOR corners[4];
			corners[0] = corner = XMVector3Transform(startPosition, DirectX::XMMatrixTranslationFromVector(right * -20));  //Bottom left
			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * 40)); //Bottom right
			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(fwd * 30)); //Top right
			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * -40)); //Top left

			//Set the pos vector back to the centre of the room on the back wall
			//This will eventually be replaced by picking a random position on a wall
			XMFLOAT3 endFloat;
			XMVECTOR endPosition;
			//Pick a random corner for the exit wall
			int endCornerOne = (rand() % 3) + 1;
			int endCornerTwo;

			//Choose the second corner based on the first random corner such that the exit wall
			//is never the same wall as the entrance wall
			//i.e. if endCornerOne == 1, endCornerTwo = 2 else if endCornerOne == 2, endCornerTwo = 3 else endCornerOne == 3, endCornerTwo = 0 
			//Also set up forward and right vectors based on which wall the exit is on
			switch (endCornerOne)
			{
			case 1:
				endFwd = right;
				endCornerTwo = 2;
				break;
			case 2:
				endFwd = fwd;
				endCornerTwo = 3;
				break;
			case 3:
				endFwd = -right;
				endCornerTwo = 0;
				break;
			}

			endRight = XMVector3Cross(up, endFwd);

			XMVECTOR endVec = corners[endCornerOne] - corners[endCornerTwo];
			XMVECTOR wallDirection = XMVector3Normalize(endVec);
			endVec = XMVector3Length(endVec);
			XMStoreFloat3(&endFloat, endVec);
			//make the exit point a random value on the selected wall with a gap of 2 cubes (each of width 2, hence the range being 6 -> wall length - 6) 
			//on either side of the wall that is not allowed to be the exit point
			int exitPoint = (rand() % ((int)endFloat.x - 12)) + 6;
			//Make sure the exitPoint is even as each cube is 2 units in width
			exitPoint = (exitPoint % 2 == 1 ? exitPoint + 1 : exitPoint);
			
			//Set the end point on the chosen wall by moving from one of the corners along the wallDirection vector
			//by the randomly chosen exitPoint value
			pos = XMVector3Transform(corners[endCornerTwo], XMMatrixTranslationFromVector(wallDirection * exitPoint));
			endPosition = pos;

			//Randomly select a height between 8 and 18, the value needs to be even as the cubes are
			//2 units on each side
			int height = (rand() % 10) + 8;
			height = (height % 2 == 1 ? height + 1 : height);

			//Build a room using the corners provided
			BuildRoom(corners, cubePos, floorCubes, endPosition, instanceCount, floorInstanceCount, height);

			//Set the new forward and right vectors based on which wall the exit was on
			fwd = endFwd;
			right = endRight;

			//Set the next start position to the current end position in the case that 2 rooms are placed one after the other
			startPosition = endPosition;
			break;
		}

		case 'E':
		{
			XMVECTOR corner;
			XMVECTOR corners[4];
			corners[0] = corner = XMVector3Transform(startPosition, DirectX::XMMatrixTranslationFromVector(right * -20));  //Bottom left
			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * 40)); //Bottom right
			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(fwd * 30)); //Top right
			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * -40)); //Top left

			//End position isn't used for the final room so its value doesn't matter
			XMVECTOR endPosition = XMVECTOR{ 0,0,0 };

			//Randomly select a height between 8 and 18, the value needs to be even as the cubes are
			//2 units on each side
			int height = (rand() % 10) + 8;
			height = (height % 2 == 1 ? height + 1 : height);

			BuildRoom(corners, cubePos, floorCubes, endPosition, instanceCount, floorInstanceCount, height);
			break;
		}
		}
	}

	//Set up the buffers required to draw all the cubes created and then clean up the arrays
	m_InstancedCube->initBuffers(renderer->getDevice(), cubePos, instanceCount);
	m_InstancedCubeFloor->initBuffers(renderer->getDevice(), floorCubes, floorInstanceCount);

	delete[] cubePos;
	delete[] floorCubes;
	cubePos = 0;
	floorCubes = 0;
}

//Corners must be in order (bottom left -> bottom right -> top right -> top left) when looking from top-down
void App1::BuildRoom(XMVECTOR* corners, XMFLOAT3* cubePositions, XMFLOAT3* cubeFloorPositions, XMVECTOR endPos, int& cubeInstances, int& cubeFloorInstances, int height)
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

	XMFLOAT3 posFloat;
	XMFLOAT3 endPosFloat;
	XMFLOAT3 startPosFloat;

	XMStoreFloat3(&endPosFloat, endPos);
	XMStoreFloat3(&startPosFloat, startPosition);

	for (int i = 0; i < 4; i++)
	{
		target[i] = walls[i] + pos[i];
	}

	for (int i = 0; i < longest / 2; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			distanceLeft = target[j] - pos[j];
			distanceLeft = XMVector3Length(distanceLeft);
			distanceLeft = XMVector3Normalize(distanceLeft);

			//Pretty much this whole if statement is super janky and probably inefficient, 
			//please fix this if you have time, that said... IT WORKS!
			if (XMVector3Greater(distanceLeft, XMVECTOR{ 0,0,0 }))
			{
				//Build the floor
				if (j == 3)
				{
					XMVECTOR floorBuilder = pos[3];
					XMFLOAT3 floorPos;

					for (int k = 0; k < lengths[0].x / 2; k++)
					{
						XMStoreFloat3(&floorPos, floorBuilder);
						cubeFloorPositions[cubeFloorInstances] = floorPos;
						cubeFloorInstances++;

						floorBuilder += right * 2;
					}
				}

				//Build the walls
				for (int k = 0; k < height / 2; k++)
				{
					XMStoreFloat3(&posFloat, pos[j]);
					//calculate the components of the current position and the end/start positions
					//on the fwd axis of the end/start positions
					XMVECTOR posEndFwd = XMVectorMultiply(endFwd, pos[j]);
					XMVECTOR endPosEndFwd = XMVectorMultiply(endFwd, endPos);

					XMVECTOR posStartFwd = XMVectorMultiply(fwd, pos[j]);
					XMVECTOR startPosStartFwd = XMVectorMultiply(fwd, startPosition);
					
					//Negative right vectors will swap the comparison values
					//e.g. rightPosEnd + (endRight * 4) will become rightPosEnd - endRight * 4
					//Taking the absolute value of the right vector will fix this
					endRight = XMVectorAbs(endRight);
					XMVECTOR startRight = XMVectorAbs(right);

					//calculate the components of the current position and the end/start positions
					//on the right axis of the end/start positions
					XMVECTOR posEndRight = XMVectorMultiply(endRight, pos[j]);
					XMVECTOR endPosEndRight = XMVectorMultiply(endRight, endPos);

					XMVECTOR posStartRight = XMVectorMultiply(right, pos[j]);
					XMVECTOR startPosStartRight = XMVectorMultiply(right, startPosition);

					XMVECTOR posUp = XMVectorMultiply(up, pos[j]);
					XMVECTOR startUp = XMVectorMultiply(up, startPosition);
					XMVECTOR endUp = XMVectorMultiply(up, endPos);

					//If both points have same component on fwd axis then both points are on the same wall
					if (XMVector3Equal(posEndFwd, endPosEndFwd))
					{
						//If the current position is greater/less than the end position +/- 4 on the right axis
						//then draw a cube AND we are not in the End room, else don't draw a cube
						if ((XMVector3GreaterOrEqual(posEndRight, endPosEndRight + (endRight * 4)) || XMVector3LessOrEqual(posEndRight, endPosEndRight - (endRight * 4))) ||
							(XMVector3GreaterOrEqual(posUp, endUp + (up * 8))))
						{
							cubePositions[cubeInstances] = posFloat;
							cubeInstances++;
						}
						else if (currentSystemChar == 'E')
						{
							cubePositions[cubeInstances] = posFloat;
							cubeInstances++;
						}
					}
					else if (XMVector3Equal(posStartFwd, startPosStartFwd))
					{
						//If the current position is greater/less than the start position +/- 4 on the right axis
						//then draw a cube AND we are not in the Start room, else don't draw a cube
						if ((XMVector3GreaterOrEqual(posStartRight, startPosStartRight + (right * 4)) || XMVector3LessOrEqual(posStartRight, startPosStartRight - (right * 4))) ||
							(XMVector3GreaterOrEqual(posUp, startUp + (up * 8))))
						{
							cubePositions[cubeInstances] = posFloat;
							cubeInstances++;
						}
						else if (currentSystemChar == 'S')
						{
							cubePositions[cubeInstances] = posFloat;
							cubeInstances++;
						}
					}
					else
					{
						cubePositions[cubeInstances] = posFloat;
						cubeInstances++;
					}
					
					pos[j] += up * 2;
				}

				//Build the roof
				if (j == 3)
				{
					XMVECTOR roofBuilder = pos[3];
					XMFLOAT3 roofPos;

					for (int k = 0; k < lengths[0].x / 2; k++)
					{
						XMStoreFloat3(&roofPos, roofBuilder);
						cubePositions[cubeInstances] = roofPos;
						cubeInstances++;

						roofBuilder += right * 2;
					}
				}

				//Set the positiong back on the ground level and move it towards the target by the
				//width of one cube
				pos[j] -= up * height;
				pos[j] += (XMVector3Normalize(walls[j]) * 2);
			}
		}
	}
}

void App1::BuildTunnel(XMVECTOR direction, XMFLOAT3* cubePositions, XMFLOAT3* cubeFloorPositions, int& cubeInstances, int& cubeFloorInstances)
{
	XMFLOAT3 cubePosition;
	XMVECTOR position;
	XMFLOAT3 posFloat;

	for (int i = 0; i < 5; i++)
	{
		position = startPosition;

		position -= right * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubeFloorPositions[cubeFloorInstances] = cubePosition;
			cubeFloorInstances++;

			position += right * 2;
		}

		position += up * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			position += up * 2;
		}

		position -= right * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			position -= right * 2;
		}

		position -= up * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			position -= up * 2;
		}

		startPosition += (direction * 2);

		//XMStoreFloat3(&posFloat, position);
		//XMFLOAT2 posXY = { posFloat.x, posFloat.y };

		//position = XMVectorCeiling(position);
		//XMStoreFloat3(&posFloat, position);
		//posFloat.x = posXY.x;
		//posFloat.y = posXY.y;
		//position = XMLoadFloat3(&posFloat);
	}

	//Round the xand y values of position to the nearest integer but not the z
	//as this can cause gaps between room and tunnel
	//startPosition = XMVectorRound(startPosition);
	/*XMStoreFloat3(&posFloat, position);
	float posZ = posFloat.z;
	
	position = XMVectorRound(position);
	XMStoreFloat3(&posFloat, position);
	posFloat.z = posZ;
	position = XMLoadFloat3(&posFloat);*/
}