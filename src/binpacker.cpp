#include "binpacker.h"
#include <algorithm>
#include <map>
#include <numeric>

using namespace BinPacker;

bool Rect::IsValid() const {
	return left <= right && top <= bottom;
}

Area Bin::GetDimensions() const {
	return dimensions;
}

const std::vector<Rect>& Bin::GetEmptyRegions() const {
	return emptyRegions;
}

// Scores the result of clipping region by clip based on the number
// of spaces that would result and the amount of space remaining
int GetClipScore(Rect region, Rect clip) {
	using namespace std;

	// if out of bounds (no areas clipped)
	if (clip.left > region.right || clip.right < region.left
		|| clip.top > region.bottom || clip.bottom < region.top)
		return 0;
	else
	{
		const int score = 2
			+ (clip.left > region.left && clip.left <= region.right)
			+ (clip.top > region.top && clip.top <= region.bottom)
			+ (clip.right > region.right && clip.right <= region.left)
			+ (clip.bottom > region.bottom && clip.bottom <= region.top)
			- (clip.bottom == region.bottom && clip.top == region.top)
			- (clip.left == region.left && clip.right == region.right);

		const Rect intersection = {
			max(region.left, clip.left),
			max(region.top, clip.top),
			min(region.right, clip.right),
			min(region.bottom, clip.bottom)
		};

		unsigned int intersectionArea = (intersection.right - intersection.left + 1) * (intersection.bottom - intersection.top + 1);
		unsigned int boundsArea = (region.right - region.left + 1) * (region.bottom - region.top + 1);

		// Score is scaled by the amount of empty area that remains. (A perfect 0 if none.)
		return score*(boundsArea-intersectionArea);
	}
}

Rect Bin::TryPackArea(Area area) {
	using namespace std;

	if (area.width > 0 && area.height > 0
		&& area.width <= dimensions.width && area.height <= dimensions.height) {
		// Try to fit the new area into every corner of every empty region
		// (including 90-degree rotation) and compare the placement
		// against every empty region to see which position and orientation
		// results in the lowest clip score
		int minScore = numeric_limits<int>::max();
		Rect bestRect({1, 1, 0, 0});
		for (const Area & area : {area, {area.height, area.width}}) { // For each orientation
			for (const Rect & r : emptyRegions) {
				if (r.right - r.left >= area.width - 1 && r.bottom - r.top >= area.height - 1) {	// skip regions in which the area cannot fit
					// Test fitting in every corner
					int score;
					// NW
					Rect clip = { r.left, r.top, r.left + area.width - 1, r.top + area.height - 1 };
					score = accumulate(emptyRegions.cbegin(), emptyRegions.cend(), 0, [&clip](const int & score, const Rect & r){ return score + GetClipScore(r, clip); });
					if (score < minScore) { minScore = score; bestRect = clip; if (score==0) break; }

					// NE
					clip = { r.right - area.width + 1, r.top, r.right, r.top + area.height - 1 };
					score = accumulate(emptyRegions.cbegin(), emptyRegions.cend(), 0, [&clip](const int & score, const Rect & r){ return score + GetClipScore(r, clip); });
					if (score < minScore) { minScore = score; bestRect = clip; if (score==0) break; }

					// SW
					clip = { r.left, r.bottom - area.height + 1, r.left + area.width - 1, r.bottom };
					score = accumulate(emptyRegions.cbegin(), emptyRegions.cend(), 0, [&clip](const int & score, const Rect & r){ return score + GetClipScore(r, clip); });
					if (score < minScore) { minScore = score; bestRect = clip; if (score==0) break; }

					// SE
					clip = { r.right - area.width + 1, r.bottom - area.height + 1, r.right, r.bottom };
					score = accumulate(emptyRegions.cbegin(), emptyRegions.cend(), 0, [&clip](const int & score, const Rect & r){ return score + GetClipScore(r, clip); });
					if (score < minScore) { minScore = score; bestRect = clip; if (score==0) break; }
				}
			}
			if (minScore == 0) break;
		}

		if (bestRect.IsValid()) {
			const Rect & clip = bestRect;
			// Now remove regions that are clipped and create new empty regions of what remains.
			vector<Rect> emptyRegionsToInsert;
			for (auto i = emptyRegions.begin(); i != emptyRegions.end();) {
				if (clip.left <= i->right && clip.top <= i->bottom && clip.right >= i->left && clip.bottom >= i->top) {
					if (clip.left > i->left && clip.left <= i->right)
						emptyRegionsToInsert.emplace_back(Rect{ i->left, i->top, clip.left - 1, i->bottom });
					if (clip.top > i->top && clip.top <= i->bottom)
						emptyRegionsToInsert.emplace_back(Rect{ i->left, i->top, i->right, clip.top - 1 });
					if (clip.right < i->right && clip.right >= i->left)
						emptyRegionsToInsert.emplace_back(Rect{ clip.right + 1, i->top, i->right, i->bottom });
					if (clip.bottom < i->bottom && clip.bottom >= i->top)
						emptyRegionsToInsert.emplace_back(Rect{ i->left, clip.bottom + 1, i->right, i->bottom });
					i = emptyRegions.erase(i);
				} else {
					i++;
				}
			}

			// Erase clipped regions and insert new empty regions
			for (auto newRegion : emptyRegionsToInsert) {
				// If the new region has the same width, left position, and intersects
				// an existing region, or likewise with height, then merge them instead.
				auto i = find_if(emptyRegions.cbegin(), emptyRegions.cend(), [&newRegion](const Rect & r){
					return (newRegion.left == r.left && newRegion.right == r.right && newRegion.top <= r.bottom && newRegion.bottom >= r.top)
						|| (newRegion.top == r.top && newRegion.bottom == r.bottom && newRegion.left <= r.right && newRegion.right >= r.left);
				});
				if (i != emptyRegions.cend()) {
					newRegion = Rect{
						min(i->left, newRegion.left),
						min(i->top, newRegion.top),
						max(i->right, newRegion.right),
						max(i->bottom, newRegion.bottom)
					};
					emptyRegions.erase(i);
				}

				// Insert the new region according to its distance from the origin. (Using std::set instead of std::vector is slower. Ordering by size is less efficient.)
				emptyRegions.emplace(upper_bound(emptyRegions.cbegin(), emptyRegions.cend(), newRegion, [](const Rect& a, const Rect& b){ return a.left*a.top < b.left*b.top; }), newRegion);
			}

			return clip;
		}
	}

	return Rect{1, 1, 0, 0};
}

void Bin::ExtendDimensions(Area extension)
{
	unsigned int rightEdge = dimensions.width > 0 ? dimensions.width - 1 : 0;
	const unsigned int bottomEdge = dimensions.height > 0 ? dimensions.height - 1 : 0;
	// Extend empty regions along edges, and create a new empty region if one doesn't exist that doesn't span the whole axis
	if (extension.width > 0 && dimensions.height > 0) {
		// If one empty region spans the entire height, just expand that one to the right.
		auto i = find_if(emptyRegions.begin(), emptyRegions.end(), [rightEdge, bottomEdge](const Rect & r){ return r.right == rightEdge && r.bottom - r.top == bottomEdge; });
		if (i != emptyRegions.end()) {
			i->right += extension.width;
		} else {
			// Otherwise, expand all regions along the right edge and create a new empty region that spans the new area.
			for (auto && r : emptyRegions) {
				if (r.right == rightEdge)
					r.right += extension.width;
			}
			emptyRegions.emplace_back(Rect{ dimensions.width, 0, dimensions.width + extension.width - 1, bottomEdge });
		}
	}
	dimensions.width += extension.width;
	rightEdge += extension.width;

	if (extension.height > 0 && dimensions.width > 0) {
		auto i = find_if(emptyRegions.begin(), emptyRegions.end(), [rightEdge, bottomEdge](const Rect & r){ return r.bottom == bottomEdge && r.right - r.left == rightEdge; });
		if (i != emptyRegions.end()) {
			i->bottom += extension.height;
		} else {
			for (auto && r : emptyRegions) {
				if (r.bottom == bottomEdge)
					r.bottom += extension.height;
			}
			emptyRegions.emplace_back(Rect{ 0, dimensions.height, rightEdge, dimensions.height + extension.height - 1 });
		}
	}

	dimensions.height += extension.height;
}
