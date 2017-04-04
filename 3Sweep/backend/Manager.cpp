#include "Manager.h"



Manager::Manager()
{
	curt_solution = nullptr;
	number_of_strokes = 0;
}


Manager::~Manager()
{
	if (curt_solution) delete curt_solution;
	/*for (auto pointer : solutions)
	{
		delete pointer;
	}*/
}

void Manager::count()
{
	number_of_strokes++;
}

void Manager::update(const vec3 & point, const vec3 & camera_direction, bool is_line)
{
	if (!curt_solution)
	{
		Stroke stroke = Stroke();
		curt_solution = new Solution(camera_direction, stroke);
	}
	if (is_line) update_line(point, camera_direction);
	else update_curve(point, camera_direction);
}

void Manager::update_line(const vec3 & point, const vec3 & camera_direction)
{
	// TODO 
	curt_solution->set_camera_direction(camera_direction);
	curt_solution->add_point(point);
	curt_solution->compute();
}

void Manager::update_curve(const vec3 & point, const vec3 & camera_direction)
{
	// TODO
	//curt_solution->set_camera_direction(camera_direction);
	curt_solution->add_point(point);
}

void Manager::end()
{
	//solutions.push_back(curt_solution);
	curt_solution = nullptr;
}

//int main() {
//	Manager manager = Manager();
//	vec3 camera = vec3(0.0, 0.0, -1.0);
//	manager.update(vec3(-2, 0, 0), camera, true);
//	manager.update(vec3(2, 0, 0), camera, true);
//	manager.update(vec3(0, -1, 0), camera, true);
//	manager.update(vec3(0, -2, 0), camera, false);
//	manager.update(vec3(0, -3, 0), camera, false);
//	Circle* result = (Circle*)(manager.curt_solution->curt);
//	cout << "origin: " << result->getOrigin().x << ", " << result->getOrigin().y << ", " << result->getOrigin().z << endl;
//	cout << "radius: " << result->getRadius() << endl;
//	cout << "normal: " << result->getNormal().x << ", " << result->getNormal().y << ", " << result->getNormal().z << endl;
//	return 0;
//}