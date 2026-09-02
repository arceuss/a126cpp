#pragma once

#include "client/model/Cube.h"

// Beta 1.2: SignModel.java
class SignModel
{
public:
	Cube cube;
	Cube cube2;

public:
	SignModel();
	
	void render();
	// Compiles both cube lists so render() can be recorded inside a caller's
	// own display list, where a lazy compile would nest glNewList.
	void ensureCompiled();
	// render() baked through a matrix into an open Tesselator batch.
	void emitTransformed(Tesselator &t, const ModelMatrix &transform);
};
