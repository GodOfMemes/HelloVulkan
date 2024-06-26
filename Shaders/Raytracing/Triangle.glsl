struct Triangle
{
	VertexData vertices[3];
	vec3 normal;
	vec3 fragPosition; // Point where the ray hits
	vec2 uv;
};

// Parameters are gl_PrimitiveID and gl_GeometryIndexEXT
Triangle GetTriangle(uint primitiveID, uint geometryIndex)
{
	Triangle tri;

	MeshData mData = bda.meshReference.meshes[geometryIndex];
	uint vOffset = mData.vertexOffset;
	uint iOffset = mData.indexOffset;

	ivec3 index = ivec3(
		bda.indexReference.indices[(3 * primitiveID) + iOffset] + vOffset,
		bda.indexReference.indices[(3 * primitiveID) + iOffset + 1] + vOffset,
		bda.indexReference.indices[(3 * primitiveID) + iOffset + 2] + vOffset);

	tri.vertices[0] = bda.vertexReference.vertices[index.x];
	tri.vertices[1] = bda.vertexReference.vertices[index.y];
	tri.vertices[2] = bda.vertexReference.vertices[index.z];

	const vec3 bary = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	tri.normal = normalize(
		tri.vertices[0].normal.xyz * bary.x +
		tri.vertices[1].normal.xyz * bary.y +
		tri.vertices[2].normal.xyz * bary.z);

	// Point where the ray hits
	tri.fragPosition = normalize(
		tri.vertices[0].position.xyz * bary.x +
		tri.vertices[1].position.xyz * bary.y +
		tri.vertices[2].position.xyz * bary.z);

	tri.uv =
		vec2(tri.vertices[0].uvX, tri.vertices[0].uvY) * bary.x +
		vec2(tri.vertices[1].uvX, tri.vertices[1].uvY) * bary.y +
		vec2(tri.vertices[2].uvX, tri.vertices[2].uvY) * bary.z;

	return tri;
}
