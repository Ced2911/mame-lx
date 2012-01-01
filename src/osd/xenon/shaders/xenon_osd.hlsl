sampler tex0;

struct VS_INPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
	float4 Color : COLOR0;
	float2 TexCoord : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Scanline & Shadowmask Vertex Shader
//-----------------------------------------------------------------------------

uniform float TargetWidth;
uniform float TargetHeight;

uniform float RawWidth;
uniform float RawHeight;

uniform float WidthRatio;
uniform float HeightRatio;

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output = (VS_OUTPUT)0;
	
	Output.Position = float4(Input.Position.xyz, 1.0f);
	Output.Position.x /= TargetWidth;
	Output.Position.y /= TargetHeight;
	Output.Position.y = 1.0f - Output.Position.y;
	Output.Position.x -= 0.5f;
	Output.Position.y -= 0.5f;
	Output.Position *= float4(2.0f, 2.0f, 1.0f, 1.0f);
	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord + 0.5f / float2(TargetWidth, TargetHeight);

	//float Zoom = 32.0f;
	//Output.TexCoord /= Zoom;
	//Output.TexCoord += float2(0.175f * (1.0f - 1.0f / Zoom) / WidthRatio, 0.175f * (1.0f - 1.0f / Zoom) / HeightRatio);
	return Output;
}

float4 ps_main(PS_INPUT Input): COLOR {
    float4 Color;

    //Color = tex2D( tex0, Input.TexCoord) * Input.Color;
	Color = tex2D( tex0, Input.TexCoord);

    return  Color;
}

