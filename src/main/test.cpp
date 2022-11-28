

#include <iostream>

int grid[2] = {0};

void draw()
{
	char horizontal = 45;
	char vertical = 124;

	int boxWidth = 9;
	int boxHeight = 3;

	int totalBoxes = 3;

	int totalCols = boxWidth * totalBoxes;
	int totalRows = boxHeight * totalBoxes;

	grid[0] = totalRows;
	grid[1] = totalCols;

	std::cout << "Pix grid: " << totalRows << " x " << totalCols << "\n";	

	for (int row = 0; row < totalRows - 1; row++)
	{

		for (int col = 0; col < totalCols - 1; col++)
		{

			int currentCol = (totalCols / totalBoxes);
			int currentRow = (totalRows / totalBoxes);

			if (row == 4 && col == 12)
			{

				std::cout << "?";
				continue;
			}

			if ((col + 1) % currentCol == 0)
			{

				std::cout << vertical;
				continue;
			}

			if ((row + 1) % currentRow == 0)
			{
				std::cout << horizontal;
			}
			else
			{
				std::cout << " ";
			}
		}

		std::cout << "\n";
	}
}

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);


	draw();
	std::cout << "\n\n\n";

	return 0;
}