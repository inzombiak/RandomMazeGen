struct PixelInput
{
    float4 color : COLOR;
    float4 hpos  : SV_Position;
};

float4 main(PixelInput input) : SV_Target
{
    return input.color;
}

/*~
    RenderTargets 
    {
        RGBA8_UNORM
    }
*/

/*~
    CullMode: None
*/

/*~
    Topology: Line
*/
