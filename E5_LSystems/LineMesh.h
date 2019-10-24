#pragma once
#include "BaseMesh.h"
#include <vector>

class Line {
public:
	Line() {
		start = XMFLOAT3(0, 0, 0); end = XMFLOAT3(1, 1, 1);
	}
	Line(XMFLOAT3 s, XMFLOAT3 e) {
		start = s; end = e;
	}
	
	XMFLOAT3 start;
	XMFLOAT3 end;
};

class LineMesh :
	public BaseMesh
{
public:
	
	LineMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	~LineMesh();

	void	sendData(ID3D11DeviceContext* deviceContext, int line, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	int		GetLineCount() { return m_LineList.size(); }

	//Line manipulation
	void	AddLine(XMFLOAT3 start, XMFLOAT3 end) { m_LineList.push_back(Line(start, end)); }
	void	Clear() { m_LineList.clear(); }

private:
	VertexType*			vertices;
	unsigned long*		indices;
	std::vector<Line>	m_LineList;
	
	void	initBuffers(ID3D11Device* device);
	void	LoadLine(ID3D11DeviceContext* deviceContext, int lineNo);
};

