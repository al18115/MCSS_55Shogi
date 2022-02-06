#include "pruning.h"
#include <iostream>
#include <iomanip>

size_t Pruning::pruning(SearchNode* root, JosekiOption option, int result) {

	double pruning_border = 1.0;
	if (result > 0) {
		pruning_border = option.getD("joseki_pruning_border_win");
	}
	else if (result < 0) {
		pruning_border = option.getD("joseki_pruning_border_lose");
	}
	else {
		pruning_border = (option.getD("joseki_pruning_border_win") + option.getD("joseki_pruning_border_lose")) / 2;
	}
	pruningBorder = pruning_border;
	size_t gigaByte = 1024 * 1024 * 1024;
	size_t maxSize = option.getI("joseki_max_size") * gigaByte;
	size_t r0 = 0;
	size_t r = 0;
	size_t nodeSize = 0;

	do {
		std::cout << std::endl << "枝刈り前ノード数：" << SearchNode::getNodeCount() << std::endl;

		std::cout << "枝刈りを行います" << std::endl;
		std::cout << "枝刈り基準：" << std::scientific << std::setprecision(2) << std::uppercase << pruningBorder << " %" << std::endl;

		std::vector<SearchNode*>history;
		r = partialPruning(root, history, 1, 0);
		r0 += r;
		std::cout << "枝刈り　ノード数：" << r << std::endl;

		std::cout << "枝刈り後ノード数：" << SearchNode::getNodeCount() << std::endl << std::endl;
		nodeSize = SearchNode::getNodeCount() * sizeof(josekinode);

		//枝刈り基準を厳しくする
		pruningBorder *= 1.2;

		//容量が基準よりも多かったらもう一度枝刈りする
	} while (nodeSize > maxSize);
	return r0;
}

void Pruning::pruningMass(SearchNode* node, double mass){
	if (node->mass < mass) {
		pruningExecuter(node);
	}
	else {
		for (int i = 0; i < node->children.size(); ++i) {
			pruningMass(&node->children[i], mass);
		}
	}
}

size_t Pruning::partialPruning(SearchNode* node, std::vector<SearchNode*> &history, double select, int depth, double backupRate) {
	size_t r = 0;
	//末端ならその場で終了
	if (node->children.size() == 0) {
		return r;
	}
	double mass = node->mass;
	history.push_back(node);
	//枝刈り判定を行う
	if (isPruning(node, select, depth, backupRate)) {
		//r += node->children.size();
		 r += pruningExecuter(node);
	}
	else {
		
		if (pruning_type >= 0) {
			//実現確率の計算
			if (select != -1) {
				double CE = node->children[0].getEvaluation();
				//評価値の最大値の取得
				for (SearchNode& child : node->children) {
					if (CE < child.getEvaluation()) {
						CE = child.getEvaluation();
					}
				}
				//バックアップ温度
				double T_c = pruning_T_c;
				if (pruning_type == 1) {
					T_c += 10000 * pow(0.1, depth);
				}
				else if (pruning_type == 2) {
					T_c += 1 * pow(2, depth);
				}
				double Z = 0;
				for (SearchNode& child : node->children) {
					Z += std::exp(-(child.getEvaluation() - CE) / T_c);
				}
				for (int i = 0; i < node->children.size(); ++i) {
					//再帰的に枝刈りを行う
					SearchNode* child = &node->children[i];
					double s;
					if (leaveNodeCount == 0 || i < leaveNodeCount) {
						s = std::exp(-(child->getEvaluation() - CE) / T_c) / Z;
					}
					else {
						s = 0;
					}
					r += partialPruning(child, history, s * select, depth + 1);//選択確率はselectを消去する
				}
			}
			else {
				//再帰的に枝刈りを行う
				for (int i = 0; i < node->children.size();++i) {
					r += partialPruning(&node->children[i], history);
				}
			}
		}

	}
	return r;
}

int Pruning::pruningExecuter(SearchNode* node) {
	int r = 1;

	if (node->children.size() > 0) {
		for (int i = 0; i < node->children.size(); i++) {
			r += pruningExecuter(&(node->children[i]));
		}
		node->children.clear();
		node->restoreNode(node->move, SearchNode::State::NotExpanded, node->eval, node->mass);
	}
	return r;
}

bool Pruning::isPruning(SearchNode* node, double select, int depth, double backupRate) {
	switch (pruning_type)
	{
	case 0:
	default:
		//実現確率がpruningBorder%以下なら切り捨て
		//%で考えてるから0.01倍
		if (select < pruningBorder * 0.01) {
			return true;
		}

		break;

	}
	return false;
}
