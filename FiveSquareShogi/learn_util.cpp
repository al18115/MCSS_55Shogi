﻿#include "learn_util.h"

SearchNode* LearnUtil::choiceChildRandom(const SearchNode* const root, const double T, double pip) {
	using dn = std::pair<double, SearchNode*>;
	double CE = std::numeric_limits<double>::max();
	std::vector<dn> evals; evals.reserve(root->children.size());
	for (const auto& child : root->children) {
		if (child->isSearchable()) {
			double eval = child->getEs();
			evals.push_back(std::make_pair(eval, child));
			if (eval < CE) {
				CE = eval;
			}
		}
	}
	if (evals.empty()) {
		return nullptr;
	}
	double Z = 0;
	for (const auto& eval : evals) {
		Z += std::exp(-(eval.first - CE) / T);
	}
	pip *= Z;
	for (const auto& eval : evals) {
		pip -= std::exp(-(eval.first - CE) / T);
		if (pip <= 0) {
			return eval.second;
		}
	}
	return evals.front().second;
}

double alphabeta(Move& pmove, SearchPlayer& player, int depth, double alpha, double beta, SearchPlayer& bestplayer) {
	const auto eval = Evaluator::evaluate(player);
	if (depth <= 0) {
		return eval;
	}
	if (eval > alpha) {
		alpha = eval;
	}
	if (alpha >= beta) {
		return alpha;
	}
	auto moves = MoveGenerator::genCapMove(pmove, player.kyokumen);
	if (moves.empty() || pmove.isOute()) {
		bestplayer = player;
		return eval;
	}
	for (auto& m : moves) {
		const FeatureCache cache = player.feature.getCache();
		const koma::Koma captured = player.proceed(m);
		SearchPlayer cbestplayer = player;
		const double ceval = -alphabeta(m, player, depth - 1, -beta, -alpha, cbestplayer);
		if (ceval > alpha) {
			alpha = ceval;
			bestplayer = cbestplayer;
		}
		player.recede(m, captured, cache);
		if (alpha >= beta) {
			return alpha;
		}
	}
	return alpha;
}

SearchPlayer LearnUtil::getQSBest(const SearchNode* const root, SearchPlayer& player, const int depthlimit) {
	SearchPlayer bestplayer = player;
	if (depthlimit <= 0) {
		return player;
	}
	Move m(root->move);
	auto moves = MoveGenerator::genCapMove(m, player.kyokumen);
	if (moves.empty()) {
		return player;
	}
	double max = root->eval;
	for (auto m : moves) {
		const FeatureCache cache = player.feature.getCache();
		const koma::Koma captured = player.proceed(m);
		SearchPlayer cbestplayer = player;
		const double eval = -alphabeta(m, player, depthlimit - 1, std::numeric_limits<double>::lowest(), -max, cbestplayer);
		if (eval > max) {
			max = eval;
			bestplayer = cbestplayer;
		}
		player.recede(m, captured, cache);
	}
	return bestplayer;
}

LearnVec LearnUtil::getGrad(const SearchNode* const root, const SearchPlayer& rootplayer, bool teban, unsigned long long samplingnum) {
	const double T = SearchNode::getTeval();
	LearnVec vec;
	std::uniform_real_distribution<double> random{ 0, 1.0 };
	std::mt19937_64 engine{ std::random_device()() };
	for (unsigned long long i = 0; i < samplingnum; i++) {
		const SearchNode* node = root;
		double c = 1;
		double eval_prev = node->eval;
		SearchPlayer player = rootplayer;
		while (!node->isLeaf()) {
			node = choiceChildRandom(node, T, random(engine));
			player.proceed(node->move);
			c *= (node->eval + eval_prev) / T - 1;
			eval_prev = node->eval;
		}
		vec.addGrad(c, getQSBest(node, player, 8), teban);
	}
	vec *= 1.0 / samplingnum;
	return vec;
}

double LearnUtil::EvalToProb(const double eval) {
	return 1.0 / (1.0 + std::exp(-eval / probT));
}