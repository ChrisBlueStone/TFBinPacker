// Custom bin packing algorithm
// by Christopher Salcido
// I sat and thought "how would I pack boxes" and came up with this algorithm. It involves
// splitting regions into as few areas as possible. This is done by trying to fit items
// into existing corners of empty space. The algorithm checks every possible corner in
// which the item can fit and scores the fit based on how many rectangular empty spaces
// will result from splitting each empty space that it intersects. In the case that
// multiple candidates exist, the candidate that minimizes the amount of space left behind
// (effectively maximizing the amount of space filled at the same time) is chosen.

#include <vector>

namespace BinPacker
{
	struct Rect {
		unsigned int left, top, right, bottom;

		/// \brief Returns if the Rect object is valid.
		/// I.e. \a right is greater than or equal to \a left and \a bottom is greater than or equal to \a top.
		bool IsValid() const;
	};

	struct Area {
		unsigned int width, height;
	};

	/// \brief Class for recording available space.
	class Bin {
		public:
			/// \brief Attempts to location an optimal area in the bin for packing \a area.
			/// \return If successful, returns a \see Rect object of the location of the packed area, otherwise returns an invalid \see Rect object.
			Rect TryPackArea(Area area);
			/// \brief Increases the dimensions of the bin.
			void ExtendDimensions(Area extension);

			/// \brief Returns the dimensions of the bin, empty or not.
			Area GetDimensions() const;
			/// \brief Returns a read-only vector of \see Rect objects representative of available empty space within the bin.
			const std::vector<Rect>& GetEmptyRegions() const;
		private:
			Area dimensions = {0, 0};
			std::vector<Rect> emptyRegions;
	};
}
