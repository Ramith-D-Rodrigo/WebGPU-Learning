struct Camera {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
};

@group(0) @binding(0) var<uniform> uCamera: Camera;

@group(1) @binding(0) var<uniform> uModel: mat4x4f;

@group(2) @binding(0) var gradientTexture: texture_2d<f32>;
@group(2) @binding(1) var textureSampler: sampler;

struct VertexInput {
    	@location(0) position: vec3f,
    	@location(1) normal: vec3f,
	@location(2) uv: vec2f,
};

struct VertexOutput {
    	@builtin(position) position: vec4f,
    	// The location here does not refer to a vertex attribute, it just means
    	// that this field must be handled by the rasterizer.
    	// (It can also refer to another field of another struct that would be used
    	// as input to the fragment shader.)
    	@location(0) normal: vec3f,
    	@location(1) uv: vec2f,
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
	out.position = uCamera.projectionMatrix * uCamera.viewMatrix * uModel * vec4f(in.position, 1.0f);	
    	out.normal = in.normal;
	out.uv = in.uv;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    	let lightDirection = vec3f(0.5, -0.9, 0.1);
    	let shading = dot(lightDirection, in.normal);
	//let texCoords = vec2i(in.uv * vec2f(textureDimensions(gradientTexture)));
    	let color = textureSample(gradientTexture, textureSampler, in.uv).rgb;
    	return vec4f(color, 1.0f);
}