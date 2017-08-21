template <typename Entry, int MAX_COUNT>
class FixedArray
{
private:
	Entry Entries[MAX_COUNT];
	Entry* FreeList[MAX_COUNT];
	Entry* UsedList[MAX_COUNT];
	
	int UsedCount;
	int FreeCount;
	
public:
	FixedArray() 
	{
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			FreeList[i] = &Entries[i];
		}
		FreeCount = MAX_COUNT;
		UsedCount = 0;
	}


	Entry* Alloc()
	{
		assert(UsedCount < MAX_COUNT - 1);

		Entry *Free = FreeList[0];
		FreeList[0] = FreeList[FreeCount - 1];
		FreeCount--;

		UsedList[UsedCount++] = Free;
		return Free;
	}

	// Only used in inner loop
	void Free(size_t i)
	{
		// add to freelist 
		FreeList[FreeCount++] = UsedList[i];

		// delete from usedlist 
		UsedList[i] = UsedList[UsedCount - 1];
		UsedCount--;
	}

	// Array Operation
	Entry* operator[](int i)
	{
		return UsedList[i];
	}

	int Size()
	{
		return UsedCount;
	}
};

