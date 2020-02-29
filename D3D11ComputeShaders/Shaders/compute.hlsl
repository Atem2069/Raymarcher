#define EPSILON 0.01
#define WIDTH 640
#define HEIGHT 640
RWTexture2D<float4> sampleTexture : register(u0);

float sphereSDF(float3 pos, float rad)
{
	return length(pos) - rad;
}

float sdPlane(float3 p, float4 n)
{
	// n must be normalized
	return dot(p, n.xyz) + n.w;
}

float3 translatePosition(float3 pos, float3 translate)
{
	return pos - translate;
}

float map(float3 pos)
{
	float sdfPlane = sdPlane(pos, normalize(float4(0, 1, 0, 1)));
	float sphere0 = sphereSDF(pos, 1.0f);
	float sphere1 = sphereSDF(translatePosition(pos, float3(1.5, 0, 0)), 0.5f);
	return min(sdfPlane, min(sphere0, sphere1));
}

float3 estimateNormal(float3 pos)
{
	float3 res;
	res.x = map(float3(pos.x + EPSILON, pos.y, pos.z)) - map(float3(pos.x - EPSILON, pos.y, pos.z));
	res.y = map(float3(pos.x, pos.y + EPSILON, pos.z)) - map(float3(pos.x, pos.y - EPSILON, pos.z));
	res.z = map(float3(pos.x, pos.y, pos.z + EPSILON)) - map(float3(pos.x, pos.y, pos.z - EPSILON));
	return normalize(res);
}

float3 calculateLighting(float3 pos, float3 eye, float3 albedo)
{
	float3 norm = estimateNormal(pos);
	float3 ambient = 0.1f;

	float3 lightDir = normalize(float3(0, 50, 50) - pos);
	float3 diff = max(dot(norm, lightDir), 0.0f);

	float3 viewDir = normalize(eye - pos);
	float3 halfwayDir = normalize(lightDir + viewDir);

	float3 spec = pow(max(dot(norm, halfwayDir), 0.0f), 64);

	return (ambient + diff+spec) * albedo;
}

float3 rayDirection(float fieldOfView, float2 size, float2 fragCoord)
{
	float2 xy = fragCoord - size / 2.0;
	float z = size.y / tan(radians(fieldOfView) / 2.0);
	return normalize(float3(xy, -z));
}

float4x4 viewMatrix(float3 eye, float3 center, float3 up) {
	float3 f = normalize(center - eye);
	float3 s = normalize(cross(f, up));
	float3 u = cross(s, f);
	return float4x4(
		float4(s, 0.0),
		float4(u, 0.0),
		float4(-f, 0.0),
		float4(0.0, 0.0, 0.0, 1)
	);
}

[numthreads(8,8,1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	//float4 res = float4(0.392, 0.584, 0.929, 1.f);
	float4 res = 0;
	//float3 coord = float3(threadID) * 2.f - 600.f;
	float3 origin = float3(0, 0, 5);
	float3 ray = origin;
	float3 dir = rayDirection(90.f, float2(WIDTH, HEIGHT), float2(threadID.xy));
	float4x4 viewToWorld = viewMatrix(origin, origin + float3(0, 0, -1), float3(0, 1, 0));
	dir = mul(float4(dir, 1.f),viewToWorld).xyz;
	for (int i = 0; i < 1000; i++)
	{
		float depth = map(ray);
		if (depth < EPSILON)
		{
			//res = float4(threadID / 600.f, 1.f);
			//res = float4(estimateNormal(origin), 1.f);
			float4 light = float4(calculateLighting(ray, origin, 1.f), 1.f);
			res += light;
			break;
		}
		//if (depth < 0.25 && depth > EPSILON)
		//	res += 0.015;

		ray += dir * depth;
	}
	threadID.y = HEIGHT - threadID.y;	//Flip y due to how d3d handles textures
	sampleTexture[threadID.xy] = res;
	//sampleTexture[threadID.xy] = float4(threadID / 600.f, 1.f);
}