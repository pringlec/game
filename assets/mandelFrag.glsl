#version 330 core

in vec2 texCoord;
out vec4 colour;

//uniform sampler2D texture1;

void main() {
    //colour = texture(texture1, texCoord);
	float cx=texCoord.x, cy=texCoord.y; // location onscreen
	float x=cx, y=cy; // point to iterate
	float r=0.0; // radius of point (squared)
	
	for (int i=0;i<100;i++) //<- repeat!
	{
		float newx=x*x - y*y;
		y=2*x*y; // <- complex multiplication
		
		x=newx+cx; y=y+cy; // bias by onscreen location
		
		r=x*x+y*y; // check radius
		if (r>4.0) break;
	}
	colour=vec4(x,r*0.1,y,0);
}