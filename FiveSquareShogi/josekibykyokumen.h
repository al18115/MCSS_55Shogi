#pragma once
#include "josekioption.h"
#include "node.h"
#include "kyokumen.h"
#include "tree.h"
#include <unordered_map>
#include <algorithm>

class JosekiByKyokumen {
public:
	JosekiByKyokumen();
	JosekiOption option;

	void input(SearchTree* tree);
	void output(SearchNode* root);
private:
	//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72

	//局面分解用
	struct kyokumendata {
		SearchNode* node;
		size_t ID;
		std::vector<size_t>childrenID;
		int count = 0; //参照された回数を記録
		kyokumendata() { ID = -1; node = nullptr; }
		kyokumendata(size_t _ID, SearchNode* _node) {
			ID = _ID;
			node = _node;
		}
		kyokumendata(size_t _ID, Move move,SearchNode::State state, double eval, double mass) {
			ID = _ID;
			node = new SearchNode;
			node->restoreNode(move, state, eval, mass);
		}
	};

	struct kdkey {
		Bammen bammen;
		int hisCount;
		
		kdkey() { bammen = { 0 }; hisCount = -1; }
		kdkey(Bammen _bammen, int _hisCount) {
			bammen = _bammen;
			hisCount = _hisCount;
			return;
		}
		bool operator==(const kdkey& rhs) const {
			const kdkey& lhs = *this;
			return (lhs.hisCount == rhs.hisCount) && (lhs.bammen == rhs.bammen);
		}

		bool operator!=(const kdkey& rhs) const {
			return !(this->operator==(rhs));
		}
	};
	//ハッシュ関係
	struct Hash {
		std::size_t operator()(const kdkey& key) const {
			size_t h = key.hisCount;
			for (int i = 0; i < 35; ++i) {
				h += (size_t)key.bammen[i] * (2 * (size_t)i + 151);
			}
			return h;
		}
	};

	//局面保存用
	struct kyokumensavedata {
		size_t ID;
		int moveCount;
		Bammen bammen;
		uint16_t moveU;
		double eval;
		double mass;
		int status;
		kyokumensavedata() { ID = -1; moveCount = -1; bammen = { 0 }; moveU = -1; eval = 0; mass = 0; status = -1; }
		kyokumensavedata(size_t _ID,int _moveCount, Bammen _bammen,uint16_t _moveU, double _eval, double _mass, SearchNode::State _state) {
			ID = _ID;
			moveCount = _moveCount;
			moveU = _moveU;
			bammen = _bammen;
			eval = _eval;
			mass = _mass;
			status = (int)_state;
		}
	};
	//指し手保存用
	struct movesavedata {
		size_t parentKyokumenNumber;
		size_t childKyokmenNumber;
		movesavedata() { parentKyokumenNumber = -1; childKyokmenNumber = -1; }
		movesavedata(size_t _parentKyokumenNumber, size_t _childKyokumenNumber) {
			parentKyokumenNumber = _parentKyokumenNumber;
			childKyokmenNumber = _childKyokumenNumber;
		}
	};

	void inputBinary();
	void buildTree(SearchNode* node, size_t ID);
	void outputBinary();
	std::unordered_map<kdkey, kyokumendata, Hash> kyokumenMap;
	std::unordered_map<size_t, kdkey> keyList;
	size_t outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node);
};
