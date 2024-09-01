struct MyUniforms {
	color: vec4f,
	time: f32,
};


@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    // The location here does not refer to a vertex attribute, it just means
    // that this field must be handled by the rasterizer.
    // (It can also refer to another field of another struct that would be used
    // as input to the fragment shader.)
    @location(0) color: vec3f,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	let ratio = 800.0 / 600.0;
	var offset = vec2f(0.0);

	let angle = uMyUniforms.time; // you can multiply it go rotate faster

	// Rotate the position around the X axis by "mixing" a bit of Y and Z in
	// the original Y and Z.
	let alpha = cos(angle);
	let beta = sin(angle);
	var position = vec3f(
		in.position.x,
		alpha * in.position.y + beta * in.position.z,
		alpha * in.position.z - beta * in.position.y,
	);
	out.position = vec4f(position.x, position.y * ratio, position.z * 0.5 + 0.5, 1.0);	

	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let color = in.color * uMyUniforms.color.rgb;
    return vec4f(color, uMyUniforms.color.a);
}