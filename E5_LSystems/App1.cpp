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

	textureMgr->loadTexture(L"floor", L"res/textures/floor.png");
	textureMgr->loadTexture(L"walls", L"res/textures/walls.png");

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
	lSystem.AddRule('A', "{/&FFF}A");

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

	//Draw the cubes that make up the floors
	m_InstanceShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"walls"), light);
	m_InstancedCube->sendDataInstanced(renderer->getDeviceContext());
	m_InstanceShader->renderInstanced(renderer->getDeviceContext(), m_InstancedCube->getIndexCount(), m_InstancedCube->GetInstanceCount());

	m_InstanceShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"floor"), light);
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

	ImGui::SliderInt("Number of rooms", &startingLine, 0, 10);
	ImGui::SliderInt("Length of tunnels", &numIterate, 0, 10);

	if (ImGui::Button("Build Dungeon"))
	{
		BuildLine();
	}

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void App1::BuildLine()
{
	using namespace DirectX;

	const int width = 64;
	const int maxCubes = width * width * width * width;

	//number of cube instances
	int instanceCount = 0;
	int floorInstanceCount = 0;

	//arrays of cube positions
	XMFLOAT3* cubePos = new XMFLOAT3[maxCubes];
	XMFLOAT3* floorCubes = new XMFLOAT3[maxCubes];

	std::string line;
	int val;

	//The user can select how many rooms they want to have in the dungeon
	line += "SA";

	for (int i = 0; i < startingLine; i++)
	{
		line += "RA";
	}

	line += "E";

	lSystem.ChangeAxiom(line);
	lSystem.Run(numIterate);

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

	//Go through the L-System string and perform some action depending on which
	//character is encountered
	for (int i = 0; i < systemString.length(); i++) {
		currentSystemChar = systemString[i];

		switch (systemString[i]) {
		case 'F':
			//Set the direction based on the current forward and the rotation
			dir = XMVector3Transform(fwd, rotation);

			//Build a tunnel when F is found
			BuildTunnel(dir, cubePos, floorCubes, instanceCount, floorInstanceCount);
			break;

		case '{':
			//Save the current rotation so we can return to it later
			rotationStack.push(rotation);
			break;

		case '}':
			//Return to the last saved rotation value
			rotation = rotationStack.top();
			rotationStack.pop();

			//Reset the direction back to what it would have been here, based on the rotation
			dir = XMVector3Transform(fwd, rotation);
			break;

		case '&':
			//pick a random number between -25 and 25 then rotate around the y-axis
			//by the chosen number of degrees (i.e. turn to the left or right)
			//Rotation will only affect tunnels as rooms don't use the dir vector
			val = ((rand() % 50) - 25);

			rotation = XMMatrixRotationAxis(fwdY, AI_DEG_TO_RAD(val)) * rotation; //This one needs to be rotated in this order, for some reason...
			break;

		case '/':
			//pick a random number between -25 and 25 then rotate around the end position's right axis
			//by the chosen number of degrees (i.e. create an incline or decline)
			//Rotation will only affect tunnels as rooms don't use the dir vector
			val = ((rand() % 50) - 25);

			rotation *= XMMatrixRotationAxis(right, AI_DEG_TO_RAD(val));
			break;

		case 'S':
		{
			XMVECTOR corner;
			XMVECTOR corners[4];
			//Set the 4 corners based on the starting position
			//(Bottom left, bottom right, etc based on top down view)
			corners[0] = corner = XMVector3Transform(pos, XMMatrixTranslation(-20, 0, 0));  //Bottom left
			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslation(40, 0, 0)); //Bottom right
			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslation(0, 0, 30)); //Top right
			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslation(-40, 0, 0)); //Top left

			//A float3 and vector for the exit of the room
			XMFLOAT3 endFloat;
			XMVECTOR endPosition;

			//Pick a random corner for the exit wall between corners 1 - 3
			int endCornerOne = (rand() % 3) + 1;
			int endCornerTwo;

			//Choose the second corner based on the first random corner such that the exit wall
			//is never the same wall as the entrance wall
			//i.e. if endCornerOne == 1, endCornerTwo = 2 else if endCornerOne == 2, endCornerTwo = 3 else if endCornerOne == 3, endCornerTwo = 0 
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

			//A vector containing the exit wall
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
			startPosition = pos;
			pos = XMVector3Transform(corners[endCornerTwo], XMMatrixTranslationFromVector(wallDirection * exitPoint));
			endPosition = pos;

			//Randomly select a height between 8 and 18, the value needs to be even as the cubes are
			//2 units on each side
			int height = (rand() % 10) + 8;
			height = (height % 2 == 1 ? height + 1 : height);

			//Build a room using the corners provided
			BuildRoom(corners, cubePos, floorCubes, endPosition, instanceCount, floorInstanceCount, height);

			//Set the next room's start forward and right vectors to the current room's end forward and right vectors
			fwd = endFwd;
			right = endRight;

			//Set the next start position to the current end position so the next tunnel/room start lines up
			//with this room's end
			startPosition = endPosition;
			break;
		}

		case 'R':
		{
			XMVECTOR corner;
			XMVECTOR corners[4];
			//Set the 4 corners based on the starting position
			//(Bottom left, bottom right, etc based on top down view)
			corners[0] = corner = XMVector3Transform(startPosition, DirectX::XMMatrixTranslationFromVector(right * -20));  //Bottom left
			corners[1] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * 40)); //Bottom right
			corners[2] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(fwd * 30)); //Top right
			corners[3] = corner = XMVector3Transform(corner, XMMatrixTranslationFromVector(right * -40)); //Top left

			//A float3 and vector for the exit of the room
			XMFLOAT3 endFloat;
			XMVECTOR endPosition;

			//Pick a random corner for the exit wall between corners 1 - 3
			int endCornerOne = (rand() % 3) + 1;
			int endCornerTwo;

			//Choose the second corner based on the first random corner such that the exit wall
			//is never the same wall as the entrance wall
			//i.e. if endCornerOne == 1, endCornerTwo = 2 else if endCornerOne == 2, endCornerTwo = 3 else if endCornerOne == 3, endCornerTwo = 0 
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

			//Set the next start position to the current end position so the next tunnel/room start lines up
			//with this room's end
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

	//Store each wall of the room
	walls[0] = XMVECTOR(corners[1] - corners[0]);
	walls[1] = XMVECTOR(corners[2] - corners[1]);
	walls[2] = XMVECTOR(corners[2] - corners[3]);
	walls[3] = XMVECTOR(corners[3] - corners[0]);

	//Store the length of each wall
	XMStoreFloat3(&lengths[0], XMVector3Length(walls[0]));
	XMStoreFloat3(&lengths[1], XMVector3Length(walls[1]));
	XMStoreFloat3(&lengths[2], XMVector3Length(walls[2]));
	XMStoreFloat3(&lengths[3], XMVector3Length(walls[3]));

	float longestVector = lengths[0].x;

	//Find the longest wall in the room
	for (int i = 1; i < 4; i++)
	{
		if (longestVector < lengths[i].x)
		{
			longestVector = lengths[i].x;
		}
	}

	int longest;
	//Round the longest wall up so it's an integer value
	longest = ceil(longestVector);

	XMVECTOR pos[4];
	XMVECTOR target[4];
	XMVECTOR distanceLeft[4] = { 0,0,0 };
	XMVECTOR previousDistance[4];

	//Create pos vectors which will change as the room is built
	XMFLOAT3 roomCornerPos;
	pos[0] = corners[0];
	pos[1] = corners[1];
	pos[2] = corners[3];
	pos[3] = corners[0];

	XMFLOAT3 posFloat;
	XMFLOAT3 endPosFloat;
	XMFLOAT3 startPosFloat;

	//Store the end and start positions as float3s
	XMStoreFloat3(&endPosFloat, endPos);
	XMStoreFloat3(&startPosFloat, startPosition);

	for (int i = 0; i < 4; i++)
	{
		//Find the end point of each wall
		target[i] = walls[i] + pos[i];
	}

	//Only loop for the length of the longest wall
	for (int i = 0; i < longest; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			//Set previousDistance to the distance last used
			previousDistance[j] = distanceLeft[j];
			distanceLeft[j] = XMVectorRound(target[j]) - XMVectorRound(pos[j]);
			distanceLeft[j] = XMVector3Length(distanceLeft[j]);

			//Only continue building the wall if there is still distance between the position and the target
			//and if the distance isn't increasing(i.e. the position hasn't overshot the target)
			if (XMVector3Greater(distanceLeft[j], XMVECTOR{ 0,0,0 }) && XMVector3LessOrEqual(distanceLeft[j], previousDistance[j]))
			{
				//Only build the floor from one wall so we don't end up with 4 floors overlapping
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
					//Calculate the components of the current position and the end/start positions
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

					//Calculate the components of the current position and the end/start positions
					//on the right axis of the end/start positions
					XMVECTOR posEndRight = XMVectorMultiply(endRight, pos[j]);
					XMVECTOR endPosEndRight = XMVectorMultiply(endRight, endPos);

					XMVECTOR posStartRight = XMVectorMultiply(right, pos[j]);
					XMVECTOR startPosStartRight = XMVectorMultiply(right, startPosition);

					//Calculate the up components of pos, end and start. The up direction is the same regardless of if it's the
					//start or end
					XMVECTOR posUp = XMVectorMultiply(up, pos[j]);
					XMVECTOR startUp = XMVectorMultiply(up, startPosition);
					XMVECTOR endUp = XMVectorMultiply(up, endPos);

					//If both points have same component on fwd axis then both points are on the same wall
					if (XMVector3Equal(posEndFwd, endPosEndFwd))
					{
						//If the current position is greater/less than the end position +/- 4 on the right axis
						//then draw a cube
						//If the position is inside the end area but we're in the end room 
						//then draw a cube as the end room doesn't need a space at the end point
						//Else don't draw a cube
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
						//then draw a cube
						//If the position is inside the start area but we're in the start room 
						//then draw a cube as the start room doesn't need a space at the start point
						//Else don't draw a cube
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
					//If the point isn't on the start or end walls then alwats draw a cube
					else
					{
						cubePositions[cubeInstances] = posFloat;
						cubeInstances++;
					}
					
					//Move the position up by the size of a cube
					pos[j] += up * 2;
				}

				//Do the same with the roof as was done with the floor; only build it out from one wall
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
		//Start position is the centre of the 3 wide floor
		position = startPosition;

		//Move the position to and edge of the floor
		position -= right * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubeFloorPositions[cubeFloorInstances] = cubePosition;
			cubeFloorInstances++;

			//Build cubes along the 3 wide floor
			position += right * 2;
		}

		//Build a cube in the corners of the tunnel as gaps may appear between the walls and roof/floor
		XMStoreFloat3(&cubePosition, position);
		cubeFloorPositions[cubeFloorInstances] = cubePosition;
		cubeFloorInstances++;

		position += up * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			//Build cubes up  the 3 high wall
			position += up * 2;
		}

		//Build a cube in the corner of the tunnel
		XMStoreFloat3(&cubePosition, position);
		cubePositions[cubeInstances] = cubePosition;
		cubeInstances++;

		position -= right * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			//Build along the 3 wide roof
			position -= right * 2;
		}

		//Build a cube in the corner of the tunnel
		XMStoreFloat3(&cubePosition, position);
		cubePositions[cubeInstances] = cubePosition;
		cubeInstances++;

		position -= up * 2;
		for (int j = 0; j < 3; j++)
		{
			XMStoreFloat3(&cubePosition, position);
			cubePositions[cubeInstances] = cubePosition;
			cubeInstances++;

			//Build down the final 3 high wall
			position -= up * 2;
		}

		//Build a cube in the corner of the tunnel
		XMStoreFloat3(&cubePosition, position);
		cubePositions[cubeInstances] = cubePosition;
		cubeInstances++;

		//Move the start position in the direction that the tunnel is going
		startPosition += (direction * 2);
	}
}