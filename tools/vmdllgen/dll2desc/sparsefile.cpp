#include "sparsefile.h"
#include <algorithm>

SparseFileWritter::SparseFileWritter() : pos_(0), size_(0)
{
	BlockPtr zeroBlock(new Block);
	zeroBlock->Offset = 0;
	blocks_.push_back(std::move(zeroBlock));
}

void SparseFileWritter::Seek(size_t pos)
{
	pos_ = pos;
}

void SparseFileWritter::SetEndOfFile()
{
	size_ = pos_;
}

void SparseFileWritter::Write(const char* data, size_t dataSize)
{
	auto previous = std::lower_bound(begin(blocks_), end(blocks_), pos_, BlockComparer());
	if(previous == end(blocks_) || (*previous)->Offset != pos_) {
		if(previous != begin(blocks_))
			-- previous;
	}

	if((*previous)->Offset + (*previous)->Data.size() < pos_) {
		BlockPtr nb(new Block);
		nb->Offset = pos_;
		previous = blocks_.insert(++previous, std::move(nb));
	}

	size_t offsetInBlock = pos_ - (*previous)->Offset;
	if((*previous)->Data.size() < offsetInBlock + dataSize)
		(*previous)->Data.resize(offsetInBlock + dataSize);

	memcpy((*previous)->Data.data() + offsetInBlock, data, dataSize);

	while(previous != end(blocks_)) {

		while(true) {

			auto next = previous;
			++ next;

			if(next == end(blocks_))
				break;

			if((*next)->Offset > (*previous)->Offset + (*previous)->Data.size())
				break;

			if((*next)->Offset + (*next)->Data.size() <= (*previous)->Offset + (*previous)->Data.size())
				blocks_.erase(next);

			uint32_t offsetInNext = (*previous)->Offset + (*previous)->Data.size() - (*next)->Offset;

			uint32_t oldSize = (*previous)->Data.size();

			(*previous)->Data.resize((*next)->Offset + (*next)->Data.size() - (*previous)->Offset);

			memcpy((*previous)->Data.data() + oldSize, (*next)->Data.data() + offsetInNext, (*next)->Data.size() - offsetInNext);

			blocks_.erase(next);
		}

		++ previous;

	}

	pos_ += dataSize;
}

void SparseFileWritter::SerialToStream(std::ostream& stm)
{
	stm.seekp(0, std::ios::beg);
	SFHeader header;
	memset(&header, 0, sizeof(header));
	header.Magic = 'RAPS';
	header.FileSize = size_;
	header.DataBlockCount = blocks_.size();
	stm.write((char*)&header, sizeof(header));
	if(!stm) throw std::exception();

	size_t blockDataOffset = sizeof(SFHeader) + header.DataBlockCount * sizeof(SFDataBlock);

	for(BlockPtr& block : blocks_) {

		SFDataBlock dataBlock;
		memset(&dataBlock, 0, sizeof(dataBlock));
		dataBlock.BlockSize = block->Data.size();
		dataBlock.Offset = block->Offset;
		dataBlock.RealOffset = blockDataOffset;
		stm.write((char*)&dataBlock, sizeof(dataBlock));
		blockDataOffset += dataBlock.BlockSize;

	}

	for(BlockPtr& block : blocks_) {

		stm.write(block->Data.data(), block->Data.size());

	}
}
void SparseFileWritter::SerialToStreamUnsparse(std::ostream& stm) 
{
	char zero = 0;
	stm.seekp(0, std::ios::beg);
	for(BlockPtr& block : blocks_) {
		if(block->Offset >= size_) break;
		if(stm.tellp() < block->Offset) {
			uint32_t sizeToWrite = block->Offset - stm.tellp();
			for(int i = 0; i < sizeToWrite; ++i)
				stm.write(&zero, 1);
		}
		stm.write(block->Data.data(), block->Data.size());
	}
	if(stm.tellp() < size_) {
		uint32_t sizeToWrite = size_ - stm.tellp();
		for(int i = 0; i < sizeToWrite; ++i)
			stm.write(&zero, 1);
	}
}