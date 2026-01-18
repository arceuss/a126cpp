#include "client/model/SignModel.h"

// Beta: SignModel() (SignModel.java:7-11)
SignModel::SignModel() : cube(0, 0), cube2(0, 14)
{
	// Beta: this.cube.addBox(-12.0F, -14.0F, -1.0F, 24, 12, 2, 0.0F) (SignModel.java:8)
	cube.addBox(-12.0f, -14.0f, -1.0f, 24, 12, 2, 0.0f);
	
	// Beta: this.cube2.addBox(-1.0F, -2.0F, -1.0F, 2, 14, 2, 0.0F) (SignModel.java:10)
	cube2.addBox(-1.0f, -2.0f, -1.0f, 2, 14, 2, 0.0f);
}

void SignModel::render()
{
	// Beta: SignModel.render() - renders both cubes (SignModel.java:13-16)
	float scale = 0.0625f;  // Beta: 1.0F / 16.0F = 0.0625F
	cube.render(scale);     // Beta: this.cube.render(0.0625F) (SignModel.java:14)
	cube2.render(scale);    // Beta: this.cube2.render(0.0625F) (SignModel.java:15)
}
