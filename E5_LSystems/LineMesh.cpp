#include "LineMesh.h"

LineMesh::LineMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	vertexCount = 2;
	indexCount = 2;

	vertices = new VertexType[vertexCount];
	indices = new unsigned long[indexCount];

	//Default Data
	vertices[0].position = XMFLOAT3(0, 0, 0);
	vertices[0].texture = XMFLOAT2(0, 0);
	vertices[1].position = XMFLOAT3(1, 1, 1);
	vertices[1].texture = XMFLOAT2(1, 1);

	//Set up index list
	indices[0] = 0;
	indices[1] = 1;

	initBuffers(device);
}

//Cleanup the heightMap
LineMesh::~LineMesh() {
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}


// Set up the heightmap and create or update the appropriate buffers
void LineMesh::LoadLine( ID3D11DeviceContext* deviceContext, int lineNo) {		
	//Load the line segment into the buffer
	vertices[0].position = m_LineList[lineNo].start;
	vertices[0].texture = XMFLOAT2(0, 0);
	vertices[1].position = m_LineList[lineNo].end;
	vertices[1].texture = XMFLOAT2(1, 1);

	//Set up index list
	indices[0] = 0;
	indices[1] = 1;
	
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	//  Disable GPU access to the vertex buffer data.
	deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	//  Update the vertex buffer here.
	memcpy(mappedResource.pData, vertices, sizeof(VertexType) * vertexCount);
	//  Reenable GPU access to the vertex buffer data.
	deviceContext->Unmap(vertexBuffer, 0);
}

void LineMesh::sendData(ID3D11DeviceContext* deviceContext, int line,  D3D_PRIMITIVE_TOPOLOGY top)
{
	LoadLine(deviceContext, line);
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(top);
}

// Generate buffers
void LineMesh::initBuffers(ID3D11Device* device)
{
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	// Set up the description of the dyanmic vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	// Now create the vertex buffer.
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
}