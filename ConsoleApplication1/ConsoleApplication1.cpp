#include "pch.h"

#include <iostream>
#include <string>

using namespace System;
using namespace System::Data::SqlClient;
using namespace System::Diagnostics;
using namespace System::IO;

int main(array<System::String ^> ^args)
{
	try
	{
		String^ cn = "Data Source=40.118.3.128;Initial Catalog=EMG;Persist Security Info=True;User ID=made;Password=i1LfUa95gtNHdzJA8oRs";
		SqlConnection sqlc(cn);
		sqlc.Open();

		String^ q = "SELECT * FROM prova";
		SqlCommand command(q, % sqlc);
		SqlDataReader^ reader = command.ExecuteReader();

		// int vals = reader->GetValues();
		// std::cout << "gsdag" << std::endl;
		bool is_data = true;
		while (is_data) {
			is_data = reader->Read();
			if (is_data) {
				printf("%d %s %s \n", reader->GetInt32(0), reader->GetString(1), reader->GetString(2));
			}
		}
	}
	catch (Exception^ e)
	{
		std::cout << "Error";
	}

    return 0;
}
