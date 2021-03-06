// tensorflow_hifi.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <vector>
#include <eigen/src/eigen/Eigen/Dense>
#include "tensorflowLibrary.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include <stdexcept>

using namespace std;
using namespace tensorflow;

// Reads a model graph definition from disk, and creates a session object you
// can use to run it.
Status LoadGraph(const string& graph_file_name, std::unique_ptr<tensorflow::Session>* session) {
	tensorflow::GraphDef graph_def;
	Status load_graph_status =
		ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
	if (!load_graph_status.ok()) {
		return tensorflow::errors::NotFound("Failed to load compute graph at '",
			graph_file_name, "'");
	}
	session->reset(tensorflow::NewSession(tensorflow::SessionOptions()));
	Status session_create_status = (*session)->Create(graph_def);
	if (!session_create_status.ok()) {
		return session_create_status;
	}
	return Status::OK();
}

graphAction::graphAction() {

}


graphAction::graphAction(string pathname, std::unique_ptr<tensorflow::Session>* session) {

	Status load_graph_status = LoadGraph(pathname, &(*session));
	if (!load_graph_status.ok()) {
		LOG(ERROR) << load_graph_status;
	}

}

graphAction::~graphAction() {

}

void graphAction::close(std::unique_ptr<tensorflow::Session>* session) {

	(*session)->Close();

}

float* graphAction::getAnswer(float* input, std::unique_ptr<tensorflow::Session>* session,int numInput, int numOutput) {

	auto mapped_hifi_X_ = Eigen::TensorMap<Eigen::Tensor<float, 3, Eigen::RowMajor>>((&(float*)input)[0], 1, 120, numInput);
	auto eigen_hifi_X_ = Eigen::Tensor<float, 3, Eigen::RowMajor>(mapped_hifi_X_);

	Tensor X_hifi_(DT_FLOAT, TensorShape({ 1,120,numInput }));
	X_hifi_.tensor<float, 3>() = eigen_hifi_X_;

	std::vector<Tensor> outputs_hifi;

	// = (std::vector<float>)*input;
	TF_CHECK_OK((*session)->Run({ { "state", X_hifi_ } }, { "fred" }, {}, &outputs_hifi));
	Tensor Y_hifi_ = outputs_hifi[0];
	const auto& print_tensor = Y_hifi_.shaped<float, 3>({ 1,120,numOutput });

	Eigen::array<ptrdiff_t, 3> patch_dims;
	patch_dims[0] = 1;
	patch_dims[1] = 1;
	patch_dims[2] = numOutput;

	const auto& patch_expr = print_tensor.extract_patches(patch_dims);

	Eigen::Tensor<float, 4, Eigen::RowMajor> patch = patch_expr.eval();

	//printf("Output from the High Fidelity Graph:\n\n");
	//for (int i = 0; i < 120; i++) {
	//	std::cout << i << ": " << patch(0, 0, i, 0) << " " << patch(0, 0, i, 1) << " " << patch(0, 0, i, 2) << std::endl;
		//<< " " << patch(0, 0, 3, 2) << std::endl;
	//}

	float * ret = new float[3]{ patch(0, 0, 119, 0), patch(0, 0, 119, 1), patch(0, 0, 119, 2) };

	return ret;


}







