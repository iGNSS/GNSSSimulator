#include "stdafx.h"
#include "PseudoRangeCalculator.h"


PseudoRangeCalculator::PseudoRangeCalculator()
{
}

PseudoRangeCalculator::~PseudoRangeCalculator()
{
}

void PseudoRangeCalculator::ProcessTrajectoryFile(const char* fileNamewPath) {

	TrajectoryStream trajFileIn(fileNamewPath); //("..\\Simulator\\TrajectoryTestFiles\\TrajectoryFileExample_RinexMatch_rinexcoord_only1.txt");
	TrajectoryHeader trajHeader;
	TrajectoryData trajData;

	isTrajectoryRead = false;
	trajFileIn >> trajHeader;

	while (trajFileIn >> trajData) {
		try { 
			trajStore.addPosition(trajData); 
		} 
		catch(...){
			cout << "Reading Trajectory data was not successfull "<< endl;
		}
	}
	cout << "[FLAG: Success] Trajectory file parsing finished." << endl;
	
	isTrajectoryRead = true;
}

void PseudoRangeCalculator::ProcessEphemerisFile(const char* fileNamewPath) {

	Rinex3NavStream inavstrm;
	Rinex3NavHeader Rnavhead;
	Rinex3NavData Rnavdata;

	isEphemerisRead = false;
	inavstrm.open(fileNamewPath, ios::in);

	if ( !inavstrm.is_open()) {
		cout << "Warning : could not open file ";
		return;
	}

	inavstrm >> Rnavhead;

	while (inavstrm >> Rnavdata) {
		try {
			bceStore.addEphemeris(Rnavdata);
		}
		catch (...) {
			cout << "Error during 'addEphemeris' fnc." << endl;
			//Rnavdata.dump(cout);
		}
	}

	isEphemerisRead = true;
}

bool PseudoRangeCalculator::isSatVisible(const Position pos,const CommonTime time, const SatID satId, double& elevation) {
	Xvt xvt_data;
	return isSatVisible(pos, time, satId, elevation, xvt_data);
}

bool PseudoRangeCalculator::isSatVisible(const Position SitePos, const CommonTime time, const SatID satId, double& elevation, Xvt& xvt_data) {
	if (isEphemerisRead == false || isTrajectoryRead == false) {
		cout << "Ephemeris or Trajectory file is not processed. " << endl;
		return false;
	}

	if (bceStore.isPresent(satId) == false) {
		return false;
	}

	WGS84Ellipsoid wgs84ellmodel;

	xvt_data = bceStore.getXvt(satId, time);
	Position SatPos(xvt_data);
	/*SatPos.setEllipsoidModel(&wgs84ellmodel);
	SitePos.setEllipsoidModel(&wgs84ellmodel);
	SitePos.setReferenceFrame(wgs84ellmodel);*/
	elevation = SitePos.elevation(SatPos);

	if (elevation < this->elevationLimitinDegree) {
		return false;
	}
	return true;
}

Xvt PseudoRangeCalculator::getSatXvt(const Position pos, const CommonTime time, const SatID satId) {
	Xvt retXvt;
	double elevation;
	if (bceStore.isPresent(satId) == false) {
		throw;
	}
	try {
		this->isSatVisible(pos, time, satId, elevation, retXvt);
		if (elevation < this->elevationLimitinDegree) {
			throw -1;
		}
	}
	catch (...) {
		throw -1;
	}
	return retXvt;

}

bool PseudoRangeCalculator::calcPseudoRange(const CommonTime Tr, const SatID satId, double& psdrange) {
	Xvt PVT;
	CommonTime tx;
	tx = Tr;
	double rho;
	
	GPSWeekSecond gpsweeksecs(Tr);
	TrajectoryData trajData = trajStore.findPosition(gpsweeksecs);
	Position roverPos(trajData.pos);
	if (&trajData == NULL) {
		return false;
	}

	// Here we filter out the sats with low elevation
	try {
		PVT = this->getSatXvt(roverPos, Tr, satId);
	}
	catch (...) {
		psdrange = -1;
		return false;
	}

	tx = Tr;
	PVT = this->getSatXvt(roverPos, tx, satId);
	psdrange = this->calcPseudoRangeNaive(trajData, PVT);
	if (psdrange<0)
		return false;

	cout << std::setprecision(20) << "travel time: " << (psdrange) / this->C_MPS << endl;

	for( int i = 0;i < 5; i++){

		tx = Tr;
		tx -= psdrange / this->C_MPS;
		PVT = this->getSatXvt(roverPos, tx, satId);

		psdrange = this->calcPseudoRangeNaive(trajData, PVT);
		if (psdrange<0)
			return false;

		rho = (psdrange) / this->C_MPS;
		this->earthRotationCorrection(rho, &PVT);

		psdrange = this->calcPseudoRangeNaive(trajData, PVT);
		if (psdrange<0)
			return false;

		//cout << std::setprecision(20) << "travel time: " << (psdrange) / this->C_MPS << endl;
	}
	

	psdrange = psdrange - C_MPS * (PVT.clkbias + PVT.relcorr);

	cout << endl << "next sat "<< endl << endl;
	return true;
}

bool PseudoRangeCalculator::calcPseudoRangeTrop(const CommonTime Tr, const SatID satId, double & psdrange, TropModel* tropptr)
{
	Xvt PVT;
	CommonTime tx;
	tx = Tr;
	double rho;

	GPSWeekSecond gpsweeksecs(Tr);
	TrajectoryData trajData = trajStore.findPosition(gpsweeksecs);
	Position roverPos(trajData.pos);
	if (&trajData == NULL) {
		return false;
	}

	// Here we filter out the sats with low elevation
	try {
		PVT = this->getSatXvt(roverPos, Tr, satId);
	}
	catch (...) {
		psdrange = -1;
		return false;
	}


	tx = Tr;
	PVT = this->getSatXvt(roverPos, tx, satId);
	psdrange = this->calcPseudoRangeNaive(trajData, PVT);
	if (psdrange<0)
		return false;

	cout << std::setprecision(20) << "travel time: " << (psdrange) / this->C_MPS << endl;

	for (int i = 0;i < 5; i++) {

		tx = Tr;
		tx -= psdrange / this->C_MPS;
		PVT = this->getSatXvt(roverPos, tx, satId);

		psdrange = this->calcPseudoRangeNaive(trajData, PVT);
		if (psdrange<0)
			return false;

		rho = (psdrange) / this->C_MPS;
		this->earthRotationCorrection(rho, &PVT);

		psdrange = this->calcPseudoRangeNaive(trajData, PVT);
		if (psdrange<0)
			return false;

		psdrange += CalculateTropModelDelays(roverPos, tx, PVT, tropptr);
		//psdrange += 200000.0;	//Pseudorange Bias

		//cout << std::setprecision(20) << "travel time: " << (psdrange) / this->C_MPS << endl;
	}


	psdrange = psdrange - C_MPS * (PVT.clkbias + PVT.relcorr);

	cout << endl << "next sat " << endl << endl;
	return true;
}

void PseudoRangeCalculator::CalculateTropModelDelays(const Position recPos, const CommonTime time, const vector<SatID> satVec, TropModel * tropmdl, vector<double>& delays)
{
	Xvt satPos;
	for (auto& currentSat : satVec) {
		satPos = this->getSatXvt(recPos, time, currentSat);
		delays.push_back(tropmdl->correction(recPos, satPos, time));
	}
}

double PseudoRangeCalculator::CalculateTropModelDelays(const Position recPos, const CommonTime time, const Xvt satPos, TropModel * tropmdl)
{
	double delay;

	delay = tropmdl->correction(recPos, satPos, time);

	return delay;
}

double PseudoRangeCalculator::calcPseudoRangeNaive(const TrajectoryData trajData, const Xvt PVT) {

	double psdrange;
	Xvt satXvt;

	Triple triple;
	WGS84Ellipsoid wgs84ellmodel;

	Position satPos(PVT.x);
	satPos.setEllipsoidModel(&wgs84ellmodel);


	Position  roverPos = trajData.pos;
	roverPos.setEllipsoidModel(&wgs84ellmodel);
	Position diff = satPos - roverPos;

	diff.setEllipsoidModel(&wgs84ellmodel);
	psdrange = sqrt(pow(diff.getX(), 2) + pow(diff.getY(), 2) + pow(diff.getZ(), 2));


	return psdrange;
}

void PseudoRangeCalculator::earthRotationCorrection(const double rho, Xvt* PVT) {
	double wt, svxyz[3];
	GPSEllipsoid ell;

	wt = ell.angVelocity()*rho;             // radians
	svxyz[0] = ::cos(wt)*PVT->x[0] + ::sin(wt)*PVT->x[1];
	svxyz[1] = -::sin(wt)*PVT->x[0] + ::cos(wt)*PVT->x[1];
	svxyz[2] = PVT->x[2];

	PVT->x[0] = svxyz[0];
	PVT->x[1] = svxyz[1];
	PVT->x[2] = svxyz[2];
}