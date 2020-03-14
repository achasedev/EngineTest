
struct VInput
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : UV;
};

struct VOut
{
	float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

VOut VertexFunction(VInput input)
{
    VOut output;

    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}


float4 FragmentFunction(VOut output) : SV_TARGET
{
	//return output.color;
    return float4(output.uv.x, 0.0, output.uv.y, 1.0);
}