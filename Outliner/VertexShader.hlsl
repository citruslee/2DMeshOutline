struct VOut
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
	matrix wvp;
	float4  vSomeVectorThatMayBeNeededByASpecificShader;
	float scale;
	float fTime;
	float fSomeFloatThatMayBeNeededByASpecificShader2;
	float fSomeFloatThatMayBeNeededByASpecificShader3;
};


VOut main(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	output.position = mul(position, wvp);
	//output.position.z = 0.0f;
	output.color = vSomeVectorThatMayBeNeededByASpecificShader;

	return output;
}