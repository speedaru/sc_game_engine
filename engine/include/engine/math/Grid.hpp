#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace sc::math {
	struct GridSize {
		uint32_t rows;
		uint32_t cols;
	};

	// represents a grid 
	template <typename Cell>
	class Grid {
	public:
		Grid(GridSize size)
			: m_size(size),
			m_grid(size.rows * size.cols) {}

		const Cell& GetCell(uint32_t row, uint32_t col) const {
			// bounds check
			if (row >= m_size.rows || col >= m_size.cols) {
				throw std::out_of_range("out of bounds cell");
			}

			uint32_t idx = row * m_size.cols + col;
			return m_grid[idx];
		}

		Cell& GetCell(uint32_t row, uint32_t col) {
			// bounds check
			if (row >= m_size.rows || col >= m_size.cols) {
				throw std::out_of_range("out of bounds cell");
			}

			uint32_t idx = row * m_size.cols + col;
			return m_grid[idx];
		}

		void Clear() {
			for (auto& cell : m_grid) {
				cell.clear();
			}
		}

	private:
		GridSize m_size;
		std::vector<Cell> m_grid; // contiguous rows (size: rows * cols)
	};
}
