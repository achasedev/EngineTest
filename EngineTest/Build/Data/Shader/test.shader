
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

float4 GetAsFloats(uint byteColors)
{
    float4 asFloats;


    asFloats.x = ((byteColors & 0xFF000000) >> 24) / 255.0;
    asFloats.y = ((byteColors & 0x00FF0000) >> 16) / 255.0;
    asFloats.z = ((byteColors & 0x0000FF00) >> 8) / 255.0;
    asFloats.w = (byteColors & 0x000000FF) / 255.0;

    return asFloats;
}

float4 FragmentFunction( VOut input ) : SV_Target0
{
   // First, we sample from our texture
   float4 texColor = tAlbedo.Sample( sAlbedo, input.uv ); 

   // component wise multiply to "tint" the output

   float4 finalColor = texColor * input.color;

   // output it; 
   return finalColor; 
}