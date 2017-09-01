#include "../PM_type.h"

void	PM_type::yuSaveMat( string Name, const Mat &mm )
{
	string	Name2 = Name + ".xml";
	FileStorage	fs(Name2,FileStorage::WRITE);
	fs<<Name<<mm;
	fs.release();
}