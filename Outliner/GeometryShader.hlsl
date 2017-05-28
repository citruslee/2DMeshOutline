cbuffer cbContextData : register(b0)
{
	float4x4 uViewProjMatrix;
	float2   uViewport;
	float size;
};

struct VOut
{
	float4 m_position : SV_POSITION;
	float4 m_color : COLOR;
};

[maxvertexcount(4)]
void main(line VOut _in[2], inout TriangleStream<VOut> out_)
{
	float2 pos0 = _in[0].m_position.xy / _in[0].m_position.w;
	float2 pos1 = _in[1].m_position.xy / _in[1].m_position.w;

	float2 dir = pos0 - pos1;
	dir = normalize(float2(dir.x, dir.y * uViewport.y / uViewport.x)); // correct for aspect ratio
	float2 tng0 = float2(-dir.y, dir.x);
	float2 tng1 = tng0 * size / uViewport;
	tng0 = tng0 * size / uViewport;

	VOut ret;

	// line start
	ret.m_color = _in[0].m_color;
	ret.m_position = float4((pos0 - tng0) * _in[0].m_position.w, _in[0].m_position.zw);
	out_.Append(ret);
	ret.m_position = float4((pos0 + tng0) * _in[0].m_position.w, _in[0].m_position.zw);
	out_.Append(ret);

	// line end
	ret.m_color = _in[1].m_color;
	ret.m_position = float4((pos1 - tng1) * _in[1].m_position.w, _in[1].m_position.zw);
	out_.Append(ret);
	ret.m_position = float4((pos1 + tng1) * _in[1].m_position.w, _in[1].m_position.zw);
	out_.Append(ret);
}