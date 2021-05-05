#include <iostream>
#include <fstream>
#include "josekibykyokumen.h"
#include "usi.h"

static class StopWatch {
public:
	void start() {
		starttime = std::chrono::system_clock::now();
	}
	void print(std::string before, std::string after) {
		double elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - starttime).count();
		std::cout << before << elapsed << after << std::endl;
	}
	void print(std::string before) {
		print(before, "");
	}
	void print() {
		print("");
	}
private:
	std::chrono::system_clock::time_point starttime;
};

JosekiByKyokumen::JosekiByKyokumen(){
	option.addOption("joseki_by_kyokumen_on", "check", "true");
}

void JosekiByKyokumen::input(SearchTree *tree)
{
	inputBinary();

	for (int i = 0; i < kyokumenMap[keyList[0]].childrenID.size(); ++i) {
		std::cout << kyokumenMap[keyList[0]].childrenID[i] << std::endl;
	}

	SearchNode* root = new SearchNode;
	buildTree(root,0);

	tree->setRoot(root);
}

void JosekiByKyokumen::output(SearchNode* root)
{
	StopWatch sw;
	sw.start();

	std::vector<std::string> usitokens = {"position","startpos"};
	Kyokumen kyokumen(usitokens);
	outputRecursive(0, kyokumen, root);

	sw.print("outputRecurcive:");

	outputBinary();

	sw.print("outputBinary:");
}

void JosekiByKyokumen::inputBinary()
{
	FILE* fp;
	if (fopen_s(&fp, "joseki/testKyokumen.bin", "rb") != 0) {
		std::cout << "file open error." << std::endl;
		return;
	}

	size_t kyokumenCount;
	fread(&kyokumenCount, sizeof(size_t), 1, fp);
	std::vector<kyokumensavedata>ksd;
	ksd.resize(kyokumenCount);
	fread(&ksd[0], sizeof(kyokumensavedata), kyokumenCount, fp);

	size_t childrenCount;
	fread(&childrenCount, sizeof(size_t), 1, fp);
	std::vector<movesavedata> msd;
	msd.resize(childrenCount);
	fread(&msd[0], sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	std::vector<std::vector<size_t>> kyokumenChildren;
	kyokumenChildren.resize(kyokumenCount);
	for (size_t i = 0; i < childrenCount; ++i) {
		auto key = msd[i].parentKyokumenNumber;
		kyokumenChildren[key].push_back(msd[i].childKyokmenNumber);
	}

	for (size_t i = 0; i < kyokumenCount; ++i) {
		kdkey key = kdkey(ksd[i].bammen, ksd[i].moveCount);
		kyokumenMap[key] = kyokumendata(ksd[i].ID, Move(ksd[i].moveU), (SearchNode::State)ksd[i].status, ksd[i].eval, ksd[i].mass);
		kyokumenMap[key].childrenID = kyokumenChildren[ksd[i].ID];
		keyList[ksd[i].ID] = key;
	}

}

static int depth = 0;
void JosekiByKyokumen::buildTree(SearchNode* node, size_t ID)
{
	const auto& key = keyList[ID];
	const auto& km = kyokumenMap[key];
	//if (km->count != 0) {
	//	return;
	//}
	//km->count = 0;
	node->restoreNode(km.node->move, km.node->getState(), km.node->eval, km.node->mass);

	//std::cout << ID << std::endl;
	//std::cout << keyList.find(ID)->first << std::endl;
	//std::cout << node->move.toUSI() << std::endl;
	//std::cout << node->eval << std::endl;
	//for (int i = 0; i < km.childrenID.size(); ++i) {
	//	std::cout << km.childrenID[i] << "\t";
	//	std::cout << kyokumenMap[keyList[km.childrenID[i]]].node->move.binary() << std::endl;
	//}

	size_t childCount = km.childrenID.size();
	SearchNode* list = new SearchNode[childCount];
	for (int i = 0; i < childCount; ++i) {
		//std::cout << ID << "," << km.childrenID[i] << std::endl;
		//std::cout << depth << std::endl;
		//++depth;
		buildTree(&(list[i]), km.childrenID[i]);
		//--depth;
	}
	node->children.setChildren(list,childCount);
	//std::cout << ID << std::endl;
}

void JosekiByKyokumen::outputBinary() {
	//局面の出力
	std::vector<kyokumensavedata> ksd;
	ksd.reserve(kyokumenMap.size());
	std::vector<movesavedata>msd;

	for (auto itr = kyokumenMap.begin(); itr != kyokumenMap.end(); ++itr) {
		ksd.push_back(
			kyokumensavedata(
				itr->second.ID,
				itr->first.hisCount,
				itr->first.bammen,
				itr->second.node->move.binary(),
				itr->second.node->eval,
				itr->second.node->mass,
				itr->second.node->getState()
			)
		);
		for (int i = 0; i < itr->second.childrenID.size();++i) {
			msd.push_back(movesavedata(itr->second.ID, itr->second.childrenID[i]));
		}
		if (itr->second.ID == 0) {
			std::cout << std::endl;
		}
	}
	
	FILE* fp;
	if (fopen_s(&fp, "joseki/testkyokumen.bin", "wb") != 0) {
		std::cout << "file open error." << std::endl;
		return;
	}

	size_t kyokumenCount = ksd.size();
	fwrite(&kyokumenCount, sizeof(size_t), 1, fp);
	fwrite(&ksd[0], sizeof(kyokumensavedata), kyokumenCount, fp);

	size_t childrenCount = msd.size();
	fwrite(&childrenCount, sizeof(size_t), 1, fp);
	fwrite(&msd[0], sizeof(movesavedata), childrenCount, fp);

	fclose(fp);

	bool outputtexton = true;
	if (outputtexton) {
		std::vector<std::string>outputs;
		outputs.resize(kyokumenCount);
		std::vector<uint16_t>sashite;
		sashite.resize(kyokumenCount);
		for (size_t i = 0; i < kyokumenCount; ++i) {
			outputs[ksd[i].ID] = "";
			outputs[ksd[i].ID] += std::to_string(ksd[i].ID) + "\t";
			outputs[ksd[i].ID] += Kyokumen(ksd[i].bammen, (ksd[i].moveCount + 1) % 2).toSfen() + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].moveCount) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].status) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].eval) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].mass) + "\t";
			outputs[ksd[i].ID] += std::to_string(ksd[i].moveU) + "(" + Move(ksd[i].moveU).toUSI() + ")" + "\n";

			sashite[ksd[i].ID] = ksd[i].moveU;
		}

		for (size_t i = 0; i < childrenCount; ++i) {
			size_t ID = msd[i].parentKyokumenNumber;
			size_t childID = msd[i].childKyokmenNumber;
			outputs[ID] += std::to_string(ID) + "\t" + std::to_string(childID) + "\t" + std::to_string(sashite[childID]) + "(" + Move(sashite[childID]).toUSI() + ")" + "\n";
		}
		std::ofstream ofs("joseki/testkyokumen.txt");
		ofs << "kyokumenCount:" << kyokumenCount << std::endl;
		ofs << "childrenCount:" << childrenCount << std::endl;
		for (size_t i = 0; i < kyokumenCount; ++i) {
			ofs << outputs[i];
		}
		fclose(fp);
	}
}

static void vectorUnique(std::vector<size_t> v) {
	std::sort(v.begin(), v.end());
	v.erase(std::unique(v.begin(), v.end()), v.end());
}

size_t JosekiByKyokumen::outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node)
{
	size_t ID;

	kdkey key(kyokumen.getBammen(), hisCount);
	
	bool duplicate;

	auto itr = kyokumenMap.find(key);
	duplicate = (itr != kyokumenMap.end());

	kyokumendata* kd;
	if (duplicate) {
		kd = &kyokumenMap[key];
		ID = kd->ID;
		if (kd->node->mass < node->mass) {
			kd->node->eval.store(node->eval);
			kd->node->mass.store(node->mass);
		}
	}
	else {
		ID = kyokumenMap.size();
		kyokumendata newkd(ID, node);
		kyokumenMap[key] = newkd;
		kd = &kyokumenMap[key];
		keyList[ID] = key;
		for (int i = 0; i < node->children.size(); ++i) {
			Kyokumen childKyokumen = Kyokumen(kyokumen);
			childKyokumen.proceed(node->children[i].move);
			size_t childID = outputRecursive(hisCount + 1, childKyokumen, &(node->children[i]));
			kd->childrenID.push_back(childID);
		}
	}
	


	const Kyokumen syokikyokumen = Kyokumen(usi::split("postion sfen rbsgk/4p/5/P4/KGSBR/ b - 1", ' '));
	const Kyokumen tugikyokumen = Kyokumen(usi::split("postion sfen rbsgk/4p/5/P1B2/KGS1R/ w - 1", ' '));
	const Kyokumen errorkyokumen = Kyokumen(usi::split("postion sfen r1sgk/2b1p/5/P1B2/KGS1R/ b - 1", ' '));
	if (key == kdkey(syokikyokumen.getBammen(), 0)) {
		std::cout << kyokumen.toSfen() << std::endl;
		std::cout << "position startpos" << std::endl;
		std::cout << hisCount << std::endl;
		for (int i = 0; i < node->children.size(); ++i) {
			std::cout << kd->childrenID[i] << "\t";
			std::cout << node->children[i].move.binary() << "\t";
			std::cout << node->children[i].eval << std::endl;
		}
	}
	if (key == kdkey(tugikyokumen.getBammen(), 1)) {
		std::cout << kyokumen.toSfen() << std::endl;
		std::cout << "position startpos moves 2e3d" << std::endl;
		std::cout << hisCount << std::endl;
		for (int i = 0; i < node->children.size(); ++i) {
			std::cout << kd->childrenID[i] << "\t";
			std::cout << node->children[i].move.binary() << "\t";
			std::cout << node->children[i].eval << std::endl;
		}
	}
	if (key == kdkey(errorkyokumen.getBammen(), 2)) {
		std::cout << kyokumen.toSfen() << std::endl;
		std::cout << "position startpos moves 2e3d 4a3b" << std::endl;
		std::cout << hisCount << std::endl;
		for (int i = 0; i < node->children.size(); ++i) {
			std::cout << kd->childrenID[i] << "\t";
			std::cout << node->children[i].move.binary() << "\t";
			std::cout << node->children[i].eval << std::endl;
		}
	}

	if (ID == 0) {
		for (int i = 0; i < kyokumenMap[keyList[ID]].childrenID.size(); ++i) {
			std::cout << ID << "	" << kd->childrenID[i] << "	" << std::to_string(kyokumenMap[keyList[kyokumenMap[keyList[ID]].childrenID[i]]].node->move.binary()) + "(" + kyokumenMap[keyList[kyokumenMap[keyList[ID]].childrenID[i]]].node->move.toUSI() << ")" << std::endl;
		}
	}


	return ID;
}
