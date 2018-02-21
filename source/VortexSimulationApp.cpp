#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class VortexSimulationApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void VortexSimulationApp::setup()
{
}

void VortexSimulationApp::mouseDown( MouseEvent event )
{
}

void VortexSimulationApp::update()
{
}

void VortexSimulationApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( VortexSimulationApp, RendererGl )
