vec2 texCoord = getTexCoord0();
//texCoord = parallaxOcclusionMapping(t_Height, texCoord, -0.04);

material.baseColor.rgb = sRGBToLinear(texture(t_Albedo, texCoord).rgb);

const vec3 N = sampleNormalMap(t_Normal, texCoord);
material.normal = tangentToWorld(N, texCoord);

material.metallic = texture(t_Metallic, texCoord).r;
material.roughness = texture(t_Roughness, texCoord).r;
material.ambientOcclusion = texture(t_AO, texCoord).r;
