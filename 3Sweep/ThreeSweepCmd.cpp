#include "ThreeSweepCmd.h"
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNurbsCurve.h>
#include <cstdlib>

const char *pathFlag = "-pth", *pathLongFlag = "-path";
const char *pointFlag = "-p", *pointLongFlag = "-point";
const char *modeFlag = "-m", *modeLongFlag = "-mode";
const char *nCurvesFlag = "-ncv", *nCurvesLongFlag = "-ncurves";

Manager* ThreeSweepCmd::manager = nullptr;

ThreeSweepCmd::ThreeSweepCmd() : MPxCommand()
{
	//manager = nullptr;
}

ThreeSweepCmd::~ThreeSweepCmd()
{
	 //if (manager) delete manager;
}

MSyntax ThreeSweepCmd::newSyntax()
{
	MSyntax syntax;

	syntax.addFlag(pathFlag, pathLongFlag, MSyntax::kString);
	syntax.addFlag(modeFlag, modeLongFlag, MSyntax::kDouble);
	syntax.addFlag(nCurvesFlag, nCurvesLongFlag, MSyntax::kDouble);
	syntax.addFlag(pointFlag, pointLongFlag, MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);

	syntax.makeFlagMultiUse(pointFlag);

	return syntax;
}

MStatus ThreeSweepCmd::doIt(const MArgList& args)
{
	MString info = "";
	
	MStatus* status;

	if (!manager) manager = new Manager();

	MArgDatabase argData(newSyntax(), args);

	int mode = 0;
	int nCurves = 0;
	int subdivisionsX = 20;
	int index = 0;
	MString curGeometry = "";

	//preprocessing
	if (argData.isFlagSet(pathFlag)) {

		MString path = "";
		argData.getFlagArgument(pathFlag, 0, path);
		manager->path = path.asChar();
		
		MGlobal::displayInfo("Preprocessing: " + path);
		int retCode = preProcess(path);
		if (retCode == 0) {
			return MStatus::kSuccess;
		}
		else {
			MGlobal::displayInfo("initialize failure!");
			return MStatus::kFailure;
		}
	}

	if (argData.isFlagSet(modeFlag))
		argData.getFlagArgument(modeFlag, 0, mode);

	if (argData.isFlagSet(nCurvesFlag))
		argData.getFlagArgument(nCurvesFlag, 0, nCurves);

	index = ((nCurves - 1) / 3) + 1; //start from 1;
	curGeometry = "Geometry";
	curGeometry += index;

	//get points in this curve
	int numPoints = 0;
	MArgList pointList;
	std::vector<vec3> points;
	if (argData.isFlagSet(pointFlag)) {
		numPoints = argData.numberOfFlagUses(pointFlag);
		for (int i = 0; i < numPoints; i++) {
			unsigned index = 0;
			argData.getFlagArgumentList(pointFlag, i, pointList);
			MPoint point = pointList.asPoint(index);
			info = point.x;
			info += point.y;
			info += point.z;
			MGlobal::displayInfo("add point: " + info);
			vec3 to_add(point.x, point.y, point.z);
			points.push_back(to_add);
		}
	}

	if (manager->number_of_strokes % 3 == 0) manager->end();
	manager->number_of_strokes = nCurves;

	//initialize manager with current image dege information;
	if (manager->curt_solution == nullptr) {
		std::string s = manager->path;
		std::string delimiter = ".";
		std::string pathWitoutExtension = s.substr(0, s.find(delimiter));

		info = pathWitoutExtension.c_str();
		info += ".txt";
		MGlobal::displayInfo("Edge Path: " + info);
		vec3 camera = vec3(0.0, -1.0, 0.0);
		manager->init(camera, pathWitoutExtension + ".txt");
	}

	MString test = "TEST: ";
	test += (int)(manager->curt_solution->contours.size());
	MGlobal::displayInfo(test);

	for (int i = 0; i < numPoints; i++) {
		manager->update(points[i], true);
	}

	manager->curt_solution->compute();
	int shape = manager->curt_solution->shape;

	Geometry* plane = (Geometry*)(manager->curt_solution->curt);
	
	if (!plane) {
		MGlobal::displayInfo("Geometry is not computed");
		return MStatus::kSuccess;
	}

	if (nCurves % 3 == 2) {
		if (shape == Solution::Shape::CIRCLE) {
			MGlobal::displayInfo("Circle is computed");
			Circle* circle_plane = (Circle*)plane;
			vec3 origin = circle_plane->getOrigin();
			float radius = circle_plane->getRadius();
			vec3 tempNormal = circle_plane->getNormal();
			vec3 normal = vec3(tempNormal.z, -tempNormal.y, tempNormal.x);//switch to maya plane
			drawInitialCylinder(radius, origin, tempNormal, subdivisionsX, curGeometry);
		}
		else if (shape == Solution::Shape::SQUARE) {
			MGlobal::displayInfo("Square is computed");
			Square* square_plane = (Square*)plane;
			vec3 origin = square_plane->getOrigin();
			float length = square_plane->getLength();
			vec3 normal = square_plane->getNormal();
			vec3 rotation = vec3(0, 0, 0);
			drawInitialCube(length, origin, normal, rotation, curGeometry);
		}
		else {
			info = "this shape is not supported!";
			MGlobal::displayInfo(info);
		}
	}else if (nCurves % 3 == 0) {
		MGlobal::displayInfo("Extruding");

		//int index = nCurves/ 3; //circle index
		MString thirdStroke = "curve";
		thirdStroke += nCurves;//the third curve name

		//build temp plane
		Geometry* pre_plane = (manager->curt_solution->history[0]);
		Geometry* curt_plane = pre_plane;

		for (int i = 1; i < manager->curt_solution->history.size(); i++) {
			// Done:: change to list of circle
			curt_plane = (manager->curt_solution->history[i]);

			vec3 preNormal = pre_plane->getNormal();
			vec3 curNormal = curt_plane->getNormal();
			float scaleRatio = 0;
			if (shape == Solution::Shape::CIRCLE) {
				float preRadius = pre_plane->getRadius();
				float curRadius = curt_plane->getRadius();
				scaleRatio = (curRadius / preRadius) * (curRadius / preRadius);
				// TEST START
				MString radiusString = "Radius: ";
				radiusString += curRadius;
				radiusString += "; LastRadius: ";
				radiusString += preRadius;
				MGlobal::displayInfo(radiusString);
				// TEST END
			}
			else if (shape == Solution::Shape::SQUARE) {
				float lastLength = pre_plane->getLength();
				float curLength = curt_plane->getLength();
				scaleRatio = (curLength / lastLength) * (curLength / lastLength);
			}

			vec3 start = pre_plane->getOrigin(); 
			vec3 end = curt_plane->getOrigin();
			// update pre_circle
			pre_plane = curt_plane;
			int startIdx = subdivisionsX;
			int endIdx = subdivisionsX * 2-1;

			vec3 translateW = end - start;//word 
			float angleZ = glm::degrees(glm::acos(glm::dot(vec3(preNormal.x, preNormal.y, 0),vec3(curNormal.x, curNormal.y, 0))));
			float angleY = glm::degrees(glm::acos(glm::dot(vec3(preNormal.x, 0, preNormal.z), vec3(curNormal.x, 0, curNormal.z))));
			float angleX = glm::degrees(glm::acos(glm::dot(vec3(0, preNormal.y, preNormal.z), vec3(0, curNormal.y, curNormal.z))));
			vec3 rotationL = vec3(angleX, angleY, angleZ);

			vec3 scaleL = vec3(scaleRatio, scaleRatio, scaleRatio);
			extrude(curGeometry, startIdx, endIdx, translateW, vec3(0, 0, 0), scaleL);
	
		}
	}

	return MStatus::kSuccess;
}

void ThreeSweepCmd::drawInitialCylinder(float radius, vec3 origin, vec3 ax, int sx, MString name) {
	MString cmd = "";
	cmd = "polyCylinder -h 0.001 -r ";
	cmd += radius;
	cmd += " -sx ";
	cmd += sx;
	cmd += " -sy 1 -sz 1 -rcp 0 -cuv 3 -ch 1 -ax ";
	cmd += ax.x;
	cmd += " ";
	cmd += ax.y;
	cmd += " "; 
	cmd += ax.z;
	cmd += " -n ";
	cmd += name;
	cmd += "; move -r ";
	cmd += origin.x;
	cmd += " ";
	cmd += origin.y;
	cmd += " ";
	cmd += origin.z;

	MGlobal::displayInfo(cmd);
	MGlobal::executeCommand(cmd, true);
}

void ThreeSweepCmd::drawInitialCube(float l, vec3 origin, vec3 ax, vec3 rotation, MString name) {
	MString cmd = "";
	cmd = "polyCube -w ";
	cmd += l;
	cmd += " -h 0.0001 -d ";
	cmd += l;
	cmd += " -ax ";
	cmd += ax.x;
	cmd += " ";
	cmd += ax.y;
	cmd += " ";
	cmd += ax.z;
	cmd += " -n ";
	cmd += name;
	cmd += "; move -r ";
	cmd += origin.x;
	cmd += " ";
	cmd += origin.y;
	cmd += " ";
	cmd += origin.z;

	MGlobal::displayInfo(cmd);
	MGlobal::executeCommand(cmd, true);
}

void ThreeSweepCmd::extrude(MString objName, int startIdx, int endIdx, vec3 translate, vec3 rotation, vec3 scale) {
	
	MString cmd = "PolyExtrude; polyExtrudeFacet -constructionHistory 1 -keepFacesTogether 1 -pvx ";
	cmd += "0 -pvy 1 -pvz 0 -divisions 1 -twist 0 -taper 1 -off 0 -thickness 0 -smoothingAngle 30";
	cmd +=" -t ";
	cmd += translate.x;
	cmd += " ";
	cmd += translate.y;
	cmd += " ";
	cmd += -translate.z; 
	cmd += " -lr ";
	cmd += rotation.x;
	cmd += " ";
	cmd += rotation.y;
	cmd += " ";
	cmd += rotation.z;
	cmd += " -ls ";
	cmd += scale.x;
	cmd += " ";
	cmd += scale.y;
	cmd += " ";
	cmd += scale.z;
	cmd += " ";
	cmd += objName;
	cmd += ".f[";
	cmd += startIdx;
	cmd += ":";
	cmd += endIdx;
	cmd += "]";
	MGlobal::displayInfo(cmd);
	MGlobal::executeCommand(cmd, true);
}

std::vector<vec3> getSamplePoints(int nCurves) {
	//get sample points
	std::vector<vec3> pointsOnCurve;
	int nSample = 20;
	float step = 1.0 / 20;
	float pctParam = 0;

	MString thirdStrokeShape = "curveShape";
	thirdStrokeShape += nCurves;
	MGlobal::selectByName(thirdStrokeShape, MGlobal::kReplaceList);
	MSelectionList selected;
	MGlobal::getActiveSelectionList(selected);

	MStatus status;
	// returns the i'th selected dependency node
	MObject obj;
	selected.getDependNode(0, obj);
	if (obj.hasFn(MFn::kNurbsCurve)) {
		// Attach a function set to the selected object
		MFnNurbsCurve curveObj(obj);
		for (int i = 0; i < nSample; i++) {
			MPoint point;
			status = curveObj.getPointAtParam(pctParam, point);
			MString info;
			info = point.x;
			info += point.y;
			info += point.z;
			MGlobal::displayInfo(info);

			vec3 pointVec(point.x, point.y, point.z);
			pointsOnCurve.push_back(pointVec);

			pctParam += step;
		}
	}
	else {
		MGlobal::displayInfo("not curves!");
	}

	return pointsOnCurve;
}

int ThreeSweepCmd::preProcess(MString path) {

        std::string s = path.asChar();
        std::string delimiter = "/";
        std::string pathPre = s.substr(0, s.find_last_of(delimiter));
        std::string cmd = pathPre.append("/EdgeDetection.exe ");
        cmd.append(path.asChar());

        MString info = cmd.c_str();
        MGlobal::displayInfo(info);

        int retCode = system(cmd.c_str());
        return retCode;
}
