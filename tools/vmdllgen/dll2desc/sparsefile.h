#pragma once
#include <ostream>
#include <stdint.h>
#include <vector>
#include <memory>

struct SFHeader
{
	uint32_t		Magic; //!< 'RAPS'
	uint32_t		FileSize; //!< 未压缩的文件大小
	uint32_t		DataBlockCount; //!< 数据块个数
};

struct SFDataBlock
{
	uint32_t		BlockSize; //!< 本数据块大小
	uint32_t		Offset; //!< 在未压缩的文件中的偏移
	uint32_t		RealOffset; //!< 在本压缩文件中的偏移
};

class SparseFileWritter 
{
	struct Block
	{
		size_t				Offset;
		std::vector<char>	Data;
	};
	typedef std::unique_ptr<Block> BlockPtr;
	struct BlockComparer
	{
		bool operator () (const Block& lhs, const Block& rhs) {
			return lhs.Offset < rhs.Offset;
		}
		bool operator () (const Block& lhs, size_t offset) {
			return lhs.Offset < offset;
		}
		bool operator () (const BlockPtr& lhs, size_t offset) {
			return lhs->Offset < offset;
		}
	};
public:
	SparseFileWritter();
	void Seek(size_t);
	void Write(const char*, size_t);
	void SetEndOfFile();
	void SerialToStream(std::ostream&);
	void SerialToStreamUnsparse(std::ostream&);
private:
	std::vector<BlockPtr>		blocks_;
	size_t						pos_;
	size_t						size_;
};