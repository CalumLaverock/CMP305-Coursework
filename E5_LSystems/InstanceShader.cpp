#include "InstanceShader.h"

InstanceShader::InstanceShader( ID3D11Device* device, HWND hwnd ) : BaseShader( device, hwnd ) {
	initShader( L"instance_vs.cso", L"instance_ps.cso" );
}


InstanceShader::~InstanceShader() {
	// Release the sampler state.
	if( sampleState ) {
		sampleState->Release();
		sampleState = 0;
	}

	// Release the matrix constant buffer.
	if( matrixBuffer ) {
		matrixBuffer->Release();
		matrixBuffer = 0;
	}

	// Release the layout.
	if( layout ) {
		layout->Release();
		layout = 0;
	}

	// Release the light constant buffer.
	if( lightBuffer ) {
		lightBuffer->Release();
		lightBuffer = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}

void InstanceShader::initShader( const wchar_t* vsFilename, const wchar_t* psFilename ) {
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3];

	// Load (+ compile) shader files
	loadVertexShader( vsFilename );
	loadPixelShader( psFilename );

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof( MatrixBufferType );
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer( &matrixBufferDesc, NULL, &matrixBuffer );

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState( &samplerDesc, &sampleState );

	// Setup light buffer
	// Setup the description of the light dynamic constant buffer that is in the pixel shader.
	// Note that ByteWidth always needs to be a multiple of 16 if using D3D11_BIND_CONSTANT_BUFFER or CreateBuffer will fail.
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof( LightBufferType );
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer( &lightBufferDesc, NULL, &lightBuffer );

}


void InstanceShader::setShaderParameters( ID3D11DeviceContext* deviceContext, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, ID3D11ShaderResourceView* texture, Light* light ) {
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;

	XMMATRIX tworld, tview, tproj;


	// Transpose the matrices to prepare them for the shader.
	tworld = XMMatrixTranspose( worldMatrix );
	tview = XMMatrixTranspose( viewMatrix );
	tproj = XMMatrixTranspose( projectionMatrix );
	result = deviceContext->Map( matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap( matrixBuffer, 0 );
	deviceContext->VSSetConstantBuffers( 0, 1, &matrixBuffer );

	//Additional
	// Send light data to pixel shader
	LightBufferType* lightPtr;
	deviceContext->Map( lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	lightPtr = (LightBufferType*)mappedResource.pData;
	lightPtr->ambient = light->getAmbientColour();
	lightPtr->diffuse = light->getDiffuseColour();
	lightPtr->direction = light->getDirection();
	lightPtr->padding = 0.0f;
	deviceContext->Unmap( lightBuffer, 0 );
	deviceContext->PSSetConstantBuffers( 0, 1, &lightBuffer );

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources( 0, 1, &texture );
	deviceContext->PSSetSamplers( 0, 1, &sampleState );
}

// De/Activate shader stages and send shaders to GPU.
// Notice we call "DrawIndexedInstanced" here instead of "DrawIndexed"
void InstanceShader::renderInstanced( ID3D11DeviceContext* deviceContext, int indexCount, int instanceCount ) {
	// Set the vertex input layout.
	deviceContext->IASetInputLayout( layout );

	// Set the vertex and pixel shaders that will be used to render.
	deviceContext->VSSetShader( vertexShader, NULL, 0 );
	deviceContext->PSSetShader( pixelShader, NULL, 0 );
	deviceContext->CSSetShader( NULL, NULL, 0 );

	// if Hull shader is not null then set HS and DS
	if( hullShader ) {
		deviceContext->HSSetShader( hullShader, NULL, 0 );
		deviceContext->DSSetShader( domainShader, NULL, 0 );
	}
	else {
		deviceContext->HSSetShader( NULL, NULL, 0 );
		deviceContext->DSSetShader( NULL, NULL, 0 );
	}

	// if geometry shader is not null then set GS
	if( geometryShader ) {
		deviceContext->GSSetShader( geometryShader, NULL, 0 );
	}
	else {
		deviceContext->GSSetShader( NULL, NULL, 0 );
	}

	// Render the triangle.
	deviceContext->DrawIndexedInstanced( indexCount, instanceCount, 0, 0, 0 );
}

//This is different for Instancing as we need to pass through additional information in the polygon layout
void InstanceShader::loadVertexShader( const wchar_t* filename ) {
	ID3DBlob* vertexShaderBuffer;

	unsigned int numElements;

	vertexShaderBuffer = 0;

	// check file extension for correct loading function.
	std::wstring fn( filename );
	std::string::size_type idx;
	std::wstring extension;

	idx = fn.rfind( '.' );

	if( idx != std::string::npos ) {
		extension = fn.substr( idx + 1 );
	}
	else {
		// No extension found
		MessageBox( hwnd, L"Error finding vertex shader file", L"ERROR", MB_OK );
		exit( 0 );
	}

	// Load the texture in.
	if( extension != L"cso" ) {
		MessageBox( hwnd, L"Incorrect vertex shader file type", L"ERROR", MB_OK );
		exit( 0 );
	}

	// Reads compiled shader into buffer (bytecode).
	HRESULT result = D3DReadFileToBlob( filename, &vertexShaderBuffer );
	if( result != S_OK ) {
		MessageBox( NULL, filename, L"File ERROR", MB_OK );
		exit( 0 );
	}

	// Create the vertex shader from the buffer.
	renderer->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the MeshClass and in the shader.
	D3D11_INPUT_ELEMENT_DESC polygonLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

	// Get a count of the elements in the layout.
	numElements = sizeof( polygonLayout ) / sizeof( polygonLayout[0] );

	// Create the vertex input layout.
	renderer->CreateInputLayout( polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &layout );

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;
}