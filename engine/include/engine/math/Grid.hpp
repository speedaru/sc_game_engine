#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

#include <SFML/System/Vector2.hpp>

namespace sc::math {
	struct GridCell {
		uint32_t row;
		uint32_t col;

		bool operator==(GridCell other) const { return row == other.row && col == other.col; }
	};

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

		Cell* TryGetCell(GridCell cell) {
			// bounds check
			if (cell.row >= m_size.rows || cell.col >= m_size.cols) {
				return nullptr;
			}

			uint32_t idx = cell.row * m_size.cols + cell.col;
			return &m_grid[idx];
		}

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
