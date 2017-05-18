struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix mWorldViewProj;
	float4  vSomeVectorThatMayBeNeededByASpecificShader;
	float fSomeFloatThatMayBeNeededByASpecificShader;
	float fTime;
	float fSomeFloatThatMayBeNeededByASpecificShader2;
	float fSomeFloatThatMayBeNeededByASpecificShader3;
};


VOut main(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	output.position = position;
	output.color = vSomeVectorThatMayBeNeededByASpecificShader;

	return output;
}