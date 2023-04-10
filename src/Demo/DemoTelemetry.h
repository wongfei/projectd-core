#pragma once

struct TelemetryNode
{
	D::TyreStatus status[2];
	float steerTorq[2];
	float steerPos;
	float mzCurrent;
	float lastFF;
};

static std::vector<TelemetryNode> _telemetry;

static void record_telemetry()
{
	TelemetryNode node;
	node.status[0] = car_->tyres[0]->status;
	node.status[1] = car_->tyres[1]->status;
	node.steerTorq[0] = car_->suspensions[0]->getSteerTorque();
	node.steerTorq[1] = car_->suspensions[1]->getSteerTorque();
	node.steerPos = car_->lastSteerPosition;
	node.mzCurrent = car_->mzCurrent;
	node.lastFF = car_->lastFF;
	_telemetry.emplace_back(node);
}

static void dump_telemetry()
{
	FILE* fd = fopen("telemetry.csv", "wt");
	fprintf(fd, "L0; FY0; MZ0; ST0; L1; FY1; MZ1; ST1; SP; MZ; FF\n");
	for (const auto& t : _telemetry)
	{
		fprintf(fd, "%f; %f; %f; %f; %f; %f; %f; %f; %f; %f; %f\n", 
			t.status[0].load, t.status[0].Fy, t.status[0].Mz, t.steerTorq[0],
			t.status[1].load, t.status[1].Fy, t.status[1].Mz, t.steerTorq[1],
			t.steerPos, t.mzCurrent, t.lastFF
		);
	}
	fclose(fd);
}
