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
	Move getBestMove(std::vector<SearchNode*>history);
private:
	//https ://qiita.com/sokutou-metsu/items/6017a64b264ff023ec72

	//局面分解用
	struct kyokumendata {
		double eval;
		double mass;
		SearchNode::State state;
		size_t ID;
		int count = 0;
		struct child {
			uint16_t moveU;
			size_t ID;
			child():moveU(-1),ID(-1) {}
			child(uint16_t _moveU, size_t _ID) :moveU(_moveU), ID(_ID) {}
		};
		std::vector<child>children;
		kyokumendata() :kyokumendata(-1,SearchNode::State::N,-1,-1) {}
		kyokumendata(size_t _ID, SearchNode::State _state, double _eval, double _mass) :ID(_ID), eval(_eval), mass(_mass), state(_state) { count = 0; }
	};
	struct kdkey {
		Bammen bammen;
		int hisCount;
		
		kdkey() :kdkey({ 0 },-1) {}
		kdkey(Bammen _bammen, int _hisCount): bammen(_bammen), hisCount(_hisCount) {}
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
				h += (size_t)key.bammen[i] * (3 * (size_t)i + 140);
			}
			return h;
		}
	};

	//局面保存用
	struct kyokumensavedata {
		size_t ID;
		int moveCount;
		Bammen bammen;
		double eval;
		double mass;
		int status;
		kyokumensavedata() :kyokumensavedata(-1, -1, { 0 },-1,-1,SearchNode::State::N) {}
		kyokumensavedata(size_t _ID,int _moveCount, Bammen _bammen, double _eval, double _mass, SearchNode::State _state) :ID(_ID), moveCount(_moveCount), bammen(_bammen), eval(_eval), mass(_mass), status((int)_state) {}
	};
	//指し手保存用
	struct movesavedata {
		size_t parentKyokumenNumber;
		uint16_t moveU;
		size_t childKyokmenNumber;
		movesavedata():movesavedata(-1,-1,-1) {}
		movesavedata(size_t _parentKyokumenNumber,uint16_t _moveU, size_t _childKyokumenNumber):parentKyokumenNumber(_parentKyokumenNumber),moveU(_moveU),childKyokmenNumber(_childKyokumenNumber) {}
	};

	bool inputBinary();
	void buildTree(SearchNode* node, size_t ID, uint16_t moveU,bool multiThread);
	void outputBinary();
	std::unordered_map<kdkey, kyokumendata, Hash> kyokumenMap;
	std::unordered_map<size_t, kdkey> keyList;
	kyokumendata* kyokumenDataArray;
	void outputRecursive(int hisCount, Kyokumen kyokumen, SearchNode* node,size_t *childID,bool multiThread);

};
