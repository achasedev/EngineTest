
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

Texture2D<float4> tAlbedo : register(t0);
SamplerState sAlbedo : register(s0);

VOut VertexFunction(VInput input)
{
    VOut output;

    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}


float4 FragmentFunction(VOut input) : SV_TARGET
{
	//return output.color;
    return tAlbedo.Sample(sAlbedo, input.uv);
}