#include "pch.h"
#include "bts_freeemg_manage.h"
#include <chrono>

// using namespace System;

int _tmain(int argc, _TCHAR* argv[])
{
	bts_manage_tools::bts_bm_manager btsManager;
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(4)) {
		printf("Problem connecting device");
		_gettch();
		btsManager.~bts_bm_manager();
		Sleep(1000);
		printf("Cleaned");
		return 0;
	}

	printf("Configuring the parameters of the data aquisition\n");
	if (!btsManager.ConfigureAquisition(true, 25)) {
		printf("Problem configuring the aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	Sleep(1000);

	btsManager.PrintSensorData();

	map<int, vector<bts_manage_tools::sample>> complete_vectors;
	for (vector<int>::iterator iter = btsManager.readable_channels.begin(); iter < btsManager.readable_channels.end(); iter++) {
		int ch_index = *iter;
		complete_vectors[ch_index] = vector<bts_manage_tools::sample>();
	}

	printf("Arming and starting BioDAQ device...\n");
	if (!btsManager.ArmStart()) {
		printf("Starting aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	Sleep(400);

	bts_manage_tools::batch queue_batch;
	vector<bts_manage_tools::sample> allSamples(0);

	auto begin = std::chrono::high_resolution_clock::now();
	int last = 0;
	for (int i = 0; i < 100; i++) {
		queue_batch = btsManager.CompleteDequeue(std::chrono::high_resolution_clock::now());
		if (queue_batch.channelsData.size() > 0) {
			printf("Data obtained \n");
		}
		Sleep(50);
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin);
		while (duration.count() <= last + 100) {
			now = std::chrono::high_resolution_clock::now();
			duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - begin);
		}
		last += 100;
		allSamples.insert(allSamples.end(), queue_batch.channelsData[7].begin(), queue_batch.channelsData[7].end());
		for (map<int, vector<bts_manage_tools::sample>>::iterator it2 = queue_batch.channelsData.begin(); it2 != queue_batch.channelsData.end(); it2++) {
			complete_vectors[it2->first].insert(complete_vectors[it2->first].end(), it2->second.begin(), it2->second.end());
		}
	}


	printf("Stopping");
	if (!btsManager.Stop()) {
		printf("Problem stopping");
	}
	printf("Stopped");
	printf("Cleaning");
	btsManager.Clean();
	printf("Cleaned");
	btsManager.~bts_bm_manager();
	
	try
	{
		System::String^ cn = "Data Source=40.118.3.128;Initial Catalog=EMG;Persist Security Info=True;User ID=made;Password=i1LfUa95gtNHdzJA8oRs";
		System::Data::SqlClient::SqlConnection sqlc(cn);
		sqlc.Open();
		// System::String^ q = "INSERT INTO Samples (sample_timestamp, channel, sample_index, sample_value) values (@ts , @ch, @ind, @val)";
		// System::Data::SqlClient::SqlCommand command(q, % sqlc);
		System::String^ query_str = "INSERT INTO Samples (sample_timestamp, channel, sample_index, sample_value) values ";
		bool initial = true;
		int count_i = 1;
		for (map<int, vector<bts_manage_tools::sample>>::iterator it3 = complete_vectors.begin(); it3 != complete_vectors.end(); it3++) {
			for (vector<bts_manage_tools::sample>::iterator it4 = it3->second.begin(); it4 < it3->second.end(); it4++) {
				// System::String^ q = "INSERT INTO Samples (sample_timestamp, channel, sample_index, sample_value) values (@ts , @ch, @ind, @val)";
				System::String^ lin = System::String::Format("({0} , {1}, {2}, {3})", "11-10-25", it3->first, it4->index, it4->value);
				if (initial) {
					initial = false;
					query_str = System::String::Concat(query_str, lin);
					// System::Console::Write(query_str);
				}
				else {
					query_str = System::String::Concat(query_str, " , ", lin);
					if (count_i > 999) {
						System::Data::SqlClient::SqlCommand command(query_str, % sqlc);
						int ex_rows = command.ExecuteNonQuery();
						printf("affected rows %d \n", ex_rows);
						query_str = "INSERT INTO Samples (sample_timestamp, channel, sample_index, sample_value) values ";
						initial = true;
						count_i = 0;
					}
					// query_str = System::String::Concat(query_str, " , ", lin);
				}
				count_i++;
				// query_str = query_str->Concat("");
				// System::Data::SqlClient::SqlCommand command(q, % sqlc);
				// command.Parameters->AddWithValue("@ts", "11-10-25");
				// command.Parameters->AddWithValue("@ch", it3->first);
				// command.Parameters->AddWithValue("@ind", it4->index);
				// command.Parameters->AddWithValue("@val", it4->value);

				// int ex_rows = command.ExecuteNonQuery();
				// printf("affected rows %d", ex_rows);
			}
		}
		if (!initial) {
			System::Data::SqlClient::SqlCommand command(query_str, % sqlc);
			int ex_rows = command.ExecuteNonQuery();
			printf("affected rows %d \n", ex_rows);
		}

		// System::String^ vals_str = System::String::Concat("","","");
		// command.Parameters->AddWithValue("@ts", "10-10-25");
		// command.Parameters->AddWithValue("@ch", 7);
		// command.Parameters->AddWithValue("@ind", allSamples[500].index);
		// command.Parameters->AddWithValue("@val", allSamples[500].value);

		// int ex_rows = command.ExecuteNonQuery();
		// printf("affected rows %d", ex_rows);
	}
	catch (System::Exception^ e)
	{
		std::cout << "Error";
	}
	


	return 0;
}
