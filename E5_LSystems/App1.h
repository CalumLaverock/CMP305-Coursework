// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "LightShader.h"
#include "LineMesh.h"
#include "LSystem.h"
#include "InstanceShader.h"
#include "InstancedCubeMesh.h"

using namespace DirectX;

class App1 : public BaseApplication
{
public:
	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void gui();

private:

	void BuildLine();
	void BuildCubeInstances();
	void BuildRoom(XMVECTOR*, XMFLOAT3*, XMVECTOR, int&, float);
	void BuildTunnel(XMVECTOR, XMFLOAT3*, int&);

	LightShader* shader;
	LineMesh* m_Line;

	Light* light;
	LSystem lSystem;

	InstanceShader* m_InstanceShader;
	InstancedCubeMesh* m_InstancedCube;

	int startingLine = 1;
	int numIterate = 1;
	char currentSystemChar;
	XMVECTOR fwd, up, right;
	XMVECTOR startPosition = { 0,0,0 };
};

#endif