// Light vertex shader
// Standard issue vertex shader, apply matrices, pass info to pixel shader
cbuffer MatrixBuffer : register( b0 ) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

struct InputType {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 instancePosition : TEXCOORD1;
//	float3 instanceScale : TEXCOORD2;
};

struct OutputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

OutputType main( InputType input ) {
	OutputType output;
	// Update the position of the vertices based on the data for this particular instance.

/*	input.position.x *= input.instanceScale.x;
	input.position.y *= input.instanceScale.y;
	input.position.z *= input.instanceScale.z;*/
	
	input.position.x += input.instancePosition.x;
	input.position.y += input.instancePosition.y;
	input.position.z += input.instancePosition.z;


	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul( input.position, worldMatrix );
	output.position = mul( output.position, viewMatrix );
	output.position = mul( output.position, projectionMatrix );

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Calculate the normal vector against the world matrix only and normalise.
	output.normal = mul( input.normal, ( float3x3 )worldMatrix );
	output.normal = normalize( output.normal );

	return output;
}