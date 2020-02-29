Texture2D tex : register(t0);

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(VS_OUT input) : SV_Target0
{
	//return float4(0,1,0,1);
	return tex.Load(int3(input.uv.x * 600,input.uv.y * 600,0));
}