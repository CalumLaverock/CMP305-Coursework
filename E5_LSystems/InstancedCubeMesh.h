/**
* \class Instanced Cube Mesh
*
* \brief Handles setting up the instance buffer for drawing the basic Cube mesh many, many times
*
* Inherits from Cube Mesh, Builds a simple cube with texture coordinates and normals, and sets up the instance buffer.
*
* \author Gaz Robinson
*/
#pragma once

#include "CubeMesh.h"

class InstancedCubeMesh : public CubeMesh {

	//Struct that represents the information that is passed through per-instance
	//Currently only position, but scale and rotation could also be passed through
	struct InstanceType {
		XMFLOAT3 position;
	};

public:
	InstancedCubeMesh( ID3D11Device* device, ID3D11DeviceContext* deviceContext, int lresolution, int instanceCount = 100 );
	~InstancedCubeMesh();

	int GetInstanceCount();

	void initBuffers( ID3D11Device* device, XMFLOAT3* p, int count );
	void sendDataInstanced( ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

protected:
	ID3D11Buffer *	m_instanceBuffer;
	int				m_instanceCount;
};

