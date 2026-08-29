#include "nbt/ListTag.h"

#include <stdexcept>
#include <string>

#include "java/IOUtil.h"

void ListTag::write(std::ostream &os)
{
	if (list.size() > 0)
		type = list[0]->getId();
	else
		type = TAG_Byte;

	IOUtil::writeByte(os, type);
	IOUtil::writeInt(os, list.size());
	for (const auto &tag : list)
		tag->write(os);
}

void ListTag::load(std::istream &is)
{
	type = IOUtil::readByte(is);
	const int_t size = IOUtil::readInt(is);

	// Alpha NBTTagList.java:27-35 creates a tag only when the count is
	// positive. Preserve its negative-count-as-empty behavior, but turn the
	// null dereference caused by an unknown positive element id into a clear
	// malformed-input failure.
	list.clear();
	for (int_t i = 0; i < size; ++i)
	{
		std::shared_ptr<Tag> tag(Tag::newTag(type));
		if (tag == nullptr)
			throw std::runtime_error("Invalid NBT list tag id " + std::to_string(static_cast<int_t>(type)));
		tag->load(is);
		list.push_back(std::move(tag));
	}
}

byte_t ListTag::getId() const
{
	return TAG_List;
}

jstring ListTag::toString() const
{
	return String::fromUTF8(std::to_string(list.size())) + u" entries of type " + Tag::getTagName(type);
}

void ListTag::print(const std::string &indent, std::ostream &os) const
{
	Tag::print(indent, os);
	
	os << indent << "{\n";

	std::string new_indent = indent + "  ";
	for (const auto &tag : list)
	{
		tag->print(new_indent, os);
	}

	os << indent << "}\n";
}

void ListTag::add(std::shared_ptr<Tag> tag)
{
	type = tag->getId();
	list.push_back(tag);
}

std::shared_ptr<Tag> ListTag::get(int_t i) const
{
	return list.at(i);
}

int_t ListTag::size() const
{
	return list.size();
}
