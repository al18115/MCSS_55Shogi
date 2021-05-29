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

	void init();

	struct kdkey {
		Bammen bammen;
		int hisCount;

		kdkey() :kdkey({ 0 }, -1) {}
		kdkey(Bammen _bammen, int _hisCount) : bammen(_bammen), hisCount(_hisCount) {}
		bool operator==(const kdkey& rhs) const {
			const kdkey& lhs = *this;
			return (lhs.hisCount == rhs.hisCount) && (lhs.bammen == rhs.bammen);
		}

		bool operator!=(const kdkey& rhs) const {
			return !(this->operator==(rhs));
		}
	};

	void outputQueue(SearchTree& tree);
	void inputQueue(SearchTree& tree);
	void coutFronID(size_t id);
private:
};
