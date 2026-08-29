#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "network/Packet100OpenWindow.h"
#include "network/Packet101CloseWindow.h"
#include "network/Packet102WindowClick.h"
#include "network/Packet103SetSlot.h"
#include "network/Packet104WindowItems.h"
#include "network/Packet105UpdateProgressbar.h"
#include "network/Packet106Transaction.h"
#include "tools/headless/PacketTestUtils.h"
#include "tools/headless/TestFramework.h"

namespace
{

void checkFramedId(headless::TestContext& ctx, Packet& packet, int_t expectedId)
{
	const std::vector<byte_t> framed = headless::encodeFramedPacket(packet);
	ctx.check(!framed.empty(), "framed packet is nonempty");
	if (!framed.empty())
		ctx.checkEqual(static_cast<ubyte_t>(framed.front()), expectedId, "framed packet id");
}

}

HEADLESS_TEST(packet, packet100_open_window_exact_wire)
{
	Packet100OpenWindow packet;
	packet.windowId = -128;
	packet.inventoryType = 127;
	packet.windowTitle = jstring{ u'A', u'\0', u'\u07FF' };
	packet.slotsCount = -1;

	// Alpha CFR Packet100OpenWindow.java:33-37 (DataOutputStream.writeByte/writeUTF order).
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x80), 0x7F, 0x00, 0x05, 0x41,
		static_cast<byte_t>(0xC0), static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xDF), static_cast<byte_t>(0xBF), static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet100 Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 6, "Packet100 Alpha size uses UTF-16 code units");
	ctx.checkEqual(packet.getPacketId(), 100, "Packet100 id");
	checkFramedId(ctx, packet, 100);

	// Alpha CFR Packet100OpenWindow.java:25-29 (signed readByte fields and readUTF).
	std::unique_ptr<Packet100OpenWindow> decoded =
		headless::decodePacketData<Packet100OpenWindow>(expected);
	ctx.checkEqual(decoded->windowId, -128, "Packet100 signed window id");
	ctx.checkEqual(decoded->inventoryType, 127, "Packet100 inventory type");
	ctx.checkEqual(decoded->windowTitle, packet.windowTitle, "Packet100 modified UTF title");
	ctx.checkEqual(decoded->slotsCount, -1, "Packet100 signed slot count");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet100 decode/re-encode");
}

HEADLESS_TEST(packet, packet100_modified_utf_unsigned_limit)
{
	Packet100OpenWindow maximum;
	maximum.windowId = 1;
	maximum.inventoryType = 2;
	maximum.windowTitle.assign(65535, u'A');
	maximum.slotsCount = 3;

	// DataOutputStream.writeUTF permits the full unsigned-short encoded length (0xFFFF).
	const std::vector<byte_t> encoded = headless::encodePacketData(maximum);
	ctx.checkEqual(static_cast<long long>(encoded.size()), 65540,
		"Packet100 maximum modified UTF payload length");
	ctx.check(static_cast<ubyte_t>(encoded[2]) == 0xFF &&
		static_cast<ubyte_t>(encoded[3]) == 0xFF,
		"Packet100 maximum modified UTF unsigned prefix");
	std::unique_ptr<Packet100OpenWindow> decoded =
		headless::decodePacketData<Packet100OpenWindow>(encoded);
	ctx.checkEqual(static_cast<long long>(decoded->windowTitle.size()), 65535,
		"Packet100 reads unsigned UTF length");
	ctx.checkEqual(decoded->getPacketSize(), 65538, "Packet100 maximum Alpha size");
	ctx.check(headless::encodePacketData(*decoded) == encoded,
		"Packet100 maximum modified UTF round trip");

	Packet100OpenWindow oversized;
	oversized.windowTitle.assign(65536, u'A');
	bool threw = false;
	try
	{
		(void)headless::encodePacketData(oversized);
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}
	ctx.check(threw, "Packet100 rejects modified UTF payloads larger than 65535 bytes");
}

HEADLESS_TEST(packet, packet101_close_window_exact_wire)
{
	// Alpha CFR Packet101CloseWindow.java:19-20,34-35 (value constructor and writeByte).
	Packet101CloseWindow packet(-1);
	const std::vector<byte_t> expected{ static_cast<byte_t>(0xFF) };
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet101 Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 1, "Packet101 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 101, "Packet101 id");
	checkFramedId(ctx, packet, 101);

	// Alpha CFR Packet101CloseWindow.java:29-30 (DataInputStream.readByte is signed).
	std::unique_ptr<Packet101CloseWindow> decoded =
		headless::decodePacketData<Packet101CloseWindow>(expected);
	ctx.checkEqual(decoded->windowId, -1, "Packet101 signed window id");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet101 decode/re-encode");
}

HEADLESS_TEST(packet, packet102_window_click_exact_wire_and_nullable_stack)
{
	auto stack = std::make_unique<ItemStack>(0x7FFE, -128, -2);
	ItemStack* originalStack = stack.get();
	Packet102WindowClick packet(-128, -32768, -1, true, std::move(stack),
		static_cast<short_t>(0x1234));
	ctx.check(packet.itemStack.get() == originalStack,
		"Packet102 value constructor keeps the supplied ItemStack object");

	// Alpha CFR Packet102WindowClick.java:57-69 (field order, boolean, non-null ItemStack).
	const std::vector<byte_t> nonNullExpected{
		static_cast<byte_t>(0x80), static_cast<byte_t>(0x80), 0x00,
		static_cast<byte_t>(0xFF), 0x12, 0x34, 0x01, 0x7F,
		static_cast<byte_t>(0xFE), static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), nonNullExpected,
		"Packet102 non-null Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 11, "Packet102 fixed Alpha size");
	ctx.checkEqual(packet.getPacketId(), 102, "Packet102 id");
	checkFramedId(ctx, packet, 102);

	// Alpha CFR Packet102WindowClick.java:40-53 (signed bytes and non-null ItemStack decode).
	std::unique_ptr<Packet102WindowClick> nonNullDecoded =
		headless::decodePacketData<Packet102WindowClick>(nonNullExpected);
	ctx.checkEqual(nonNullDecoded->window_Id, -128, "Packet102 signed window id");
	ctx.checkEqual(nonNullDecoded->inventorySlot, -32768, "Packet102 slot short");
	ctx.checkEqual(nonNullDecoded->mouseClick, -1, "Packet102 signed mouse button");
	ctx.checkEqual(nonNullDecoded->action, 0x1234, "Packet102 action short");
	ctx.check(nonNullDecoded->field_27050_f, "Packet102 shift boolean");
	ctx.check(nonNullDecoded->itemStack != nullptr, "Packet102 non-null decoded stack");
	if (nonNullDecoded->itemStack != nullptr)
	{
		ctx.checkEqual(nonNullDecoded->itemStack->itemID, 0x7FFE, "Packet102 item id");
		ctx.checkEqual(nonNullDecoded->itemStack->stackSize, -128, "Packet102 signed item count");
		ctx.checkEqual(nonNullDecoded->itemStack->itemDamage, -2, "Packet102 item damage");
	}
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nonNullDecoded), nonNullExpected,
		"Packet102 non-null decode/re-encode");

	// Alpha CFR Packet102WindowClick.java:58-64 (null stack is exactly short -1).
	const std::vector<byte_t> nullExpected{
		0x7F, 0x7F, static_cast<byte_t>(0xFF), 0x00,
		static_cast<byte_t>(0x80), 0x00, 0x00,
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF)
	};
	std::unique_ptr<Packet102WindowClick> nullDecoded =
		headless::decodePacketData<Packet102WindowClick>(nullExpected);
	ctx.check(nullDecoded->itemStack == nullptr, "Packet102 null decoded stack");
	ctx.checkEqual(nullDecoded->getPacketSize(), 11, "Packet102 null fixed Alpha size");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nullDecoded), nullExpected,
		"Packet102 null decode/re-encode");
}

HEADLESS_TEST(packet, packet103_set_slot_exact_wire_and_nullable_stack)
{
	Packet103SetSlot packet;
	packet.windowId = -2;
	packet.itemSlot = 0x1234;
	packet.myItemStack = std::make_shared<ItemStack>(0x0102, -1, 0x7F00);

	// Alpha CFR Packet103SetSlot.java:39-48 (slot and non-null ItemStack field order).
	const std::vector<byte_t> nonNullExpected{
		static_cast<byte_t>(0xFE), 0x12, 0x34, 0x01, 0x02,
		static_cast<byte_t>(0xFF), 0x7F, 0x00
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), nonNullExpected,
		"Packet103 non-null Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 8, "Packet103 fixed Alpha size");
	ctx.checkEqual(packet.getPacketId(), 103, "Packet103 id");
	checkFramedId(ctx, packet, 103);

	// Alpha CFR Packet103SetSlot.java:25-35 (signed window byte and stack decode).
	std::unique_ptr<Packet103SetSlot> nonNullDecoded =
		headless::decodePacketData<Packet103SetSlot>(nonNullExpected);
	ctx.checkEqual(nonNullDecoded->windowId, -2, "Packet103 signed window id");
	ctx.checkEqual(nonNullDecoded->itemSlot, 0x1234, "Packet103 slot short");
	ctx.check(nonNullDecoded->myItemStack != nullptr, "Packet103 non-null decoded stack");
	if (nonNullDecoded->myItemStack != nullptr)
	{
		ctx.checkEqual(nonNullDecoded->myItemStack->itemID, 0x0102, "Packet103 item id");
		ctx.checkEqual(nonNullDecoded->myItemStack->stackSize, -1, "Packet103 signed item count");
		ctx.checkEqual(nonNullDecoded->myItemStack->itemDamage, 0x7F00, "Packet103 item damage");
	}
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nonNullDecoded), nonNullExpected,
		"Packet103 non-null decode/re-encode");

	// Alpha CFR Packet103SetSlot.java:40-43 (null stack is exactly short -1).
	const std::vector<byte_t> nullExpected{
		static_cast<byte_t>(0x80), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFE), static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF)
	};
	std::unique_ptr<Packet103SetSlot> nullDecoded =
		headless::decodePacketData<Packet103SetSlot>(nullExpected);
	ctx.check(nullDecoded->myItemStack == nullptr, "Packet103 null decoded stack");
	ctx.checkEqual(nullDecoded->getPacketSize(), 8, "Packet103 null fixed Alpha size");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nullDecoded), nullExpected,
		"Packet103 null decode/re-encode");
}

HEADLESS_TEST(packet, packet104_window_items_exact_wire_empty_and_nonempty)
{
	Packet104WindowItems packet;
	packet.windowId = -128;
	packet.itemStack = {
		nullptr,
		std::make_shared<ItemStack>(0x1234, -128, -2),
		std::make_shared<ItemStack>(0, 127, 32767)
	};

	// Alpha CFR Packet104WindowItems.java:33-44 (array count/order and nullable ItemStacks).
	const std::vector<byte_t> nonEmptyExpected{
		static_cast<byte_t>(0x80), 0x00, 0x03,
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFF),
		0x12, 0x34, static_cast<byte_t>(0x80),
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0xFE),
		0x00, 0x00, 0x7F, 0x7F, static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), nonEmptyExpected,
		"Packet104 nonempty Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 18,
		"Packet104 Alpha size charges five bytes for every array element");
	ctx.checkEqual(packet.getPacketId(), 104, "Packet104 id");
	checkFramedId(ctx, packet, 104);

	// Alpha CFR Packet104WindowItems.java:19-29 (signed id, list count/order, null element).
	std::unique_ptr<Packet104WindowItems> nonEmptyDecoded =
		headless::decodePacketData<Packet104WindowItems>(nonEmptyExpected);
	ctx.checkEqual(nonEmptyDecoded->windowId, -128, "Packet104 signed window id");
	ctx.checkEqual(static_cast<long long>(nonEmptyDecoded->itemStack.size()), 3,
		"Packet104 decoded item count");
	if (nonEmptyDecoded->itemStack.size() == 3)
	{
		ctx.check(nonEmptyDecoded->itemStack[0] == nullptr, "Packet104 null first item");
		ctx.check(nonEmptyDecoded->itemStack[1] != nullptr, "Packet104 non-null second item");
		ctx.check(nonEmptyDecoded->itemStack[2] != nullptr, "Packet104 non-null third item");
		if (nonEmptyDecoded->itemStack[1] != nullptr)
		{
			ctx.checkEqual(nonEmptyDecoded->itemStack[1]->itemID, 0x1234, "Packet104 second item id");
			ctx.checkEqual(nonEmptyDecoded->itemStack[1]->stackSize, -128,
				"Packet104 signed second item count");
			ctx.checkEqual(nonEmptyDecoded->itemStack[1]->itemDamage, -2,
				"Packet104 second item damage");
		}
	}
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nonEmptyDecoded), nonEmptyExpected,
		"Packet104 nonempty decode/re-encode");

	Packet104WindowItems empty;
	empty.windowId = -1;
	// Alpha CFR Packet104WindowItems.java:34-36 (empty array writes a zero short count).
	const std::vector<byte_t> emptyExpected{
		static_cast<byte_t>(0xFF), 0x00, 0x00
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(empty), emptyExpected,
		"Packet104 empty Alpha payload");
	ctx.checkEqual(empty.getPacketSize(), 3, "Packet104 empty Alpha size");
	std::unique_ptr<Packet104WindowItems> emptyDecoded =
		headless::decodePacketData<Packet104WindowItems>(emptyExpected);
	ctx.check(emptyDecoded->itemStack.empty(), "Packet104 empty decoded list");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*emptyDecoded), emptyExpected,
		"Packet104 empty decode/re-encode");
}

HEADLESS_TEST(packet, packet105_update_progressbar_exact_wire)
{
	Packet105UpdateProgressbar packet;
	packet.windowId = -1;
	packet.progressBar = -32768;
	packet.progressBarValue = 32767;

	// Alpha CFR Packet105UpdateProgressbar.java:31-34 (byte, short, short order).
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0xFF), static_cast<byte_t>(0x80), 0x00,
		0x7F, static_cast<byte_t>(0xFF)
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet105 Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 5, "Packet105 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 105, "Packet105 id");
	checkFramedId(ctx, packet, 105);

	// Alpha CFR Packet105UpdateProgressbar.java:24-27 (signed readByte/readShort fields).
	std::unique_ptr<Packet105UpdateProgressbar> decoded =
		headless::decodePacketData<Packet105UpdateProgressbar>(expected);
	ctx.checkEqual(decoded->windowId, -1, "Packet105 signed window id");
	ctx.checkEqual(decoded->progressBar, -32768, "Packet105 signed progress bar");
	ctx.checkEqual(decoded->progressBarValue, 32767, "Packet105 progress value");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet105 decode/re-encode");
}

HEADLESS_TEST(packet, packet106_transaction_exact_wire_and_accepted_boolean)
{
	// Alpha CFR Packet106Transaction.java:21-24,40-43 (value constructor and canonical true byte).
	Packet106Transaction packet(-128, static_cast<short_t>(-2), true);
	const std::vector<byte_t> expected{
		static_cast<byte_t>(0x80), static_cast<byte_t>(0xFF),
		static_cast<byte_t>(0xFE), 0x01
	};
	headless::checkPacketBytes(ctx, headless::encodePacketData(packet), expected,
		"Packet106 Alpha payload");
	ctx.checkEqual(packet.getPacketSize(), 4, "Packet106 Alpha size");
	ctx.checkEqual(packet.getPacketId(), 106, "Packet106 id");
	checkFramedId(ctx, packet, 106);

	// Alpha CFR Packet106Transaction.java:33-36 (readByte != 0 accepts every nonzero byte).
	std::unique_ptr<Packet106Transaction> decoded =
		headless::decodePacketData<Packet106Transaction>(expected);
	ctx.checkEqual(decoded->windowId, -128, "Packet106 signed window id");
	ctx.checkEqual(decoded->field_20028_b, -2, "Packet106 action short");
	ctx.check(decoded->field_20030_c, "Packet106 accepted true");
	headless::checkPacketBytes(ctx, headless::encodePacketData(*decoded), expected,
		"Packet106 decode/re-encode");

	const std::vector<byte_t> nonCanonicalTrue{
		0x01, 0x12, 0x34, static_cast<byte_t>(0x80)
	};
	std::unique_ptr<Packet106Transaction> nonCanonicalDecoded =
		headless::decodePacketData<Packet106Transaction>(nonCanonicalTrue);
	ctx.check(nonCanonicalDecoded->field_20030_c,
		"Packet106 negative nonzero accepted byte decodes true");
	const std::vector<byte_t> canonicalizedTrue{ 0x01, 0x12, 0x34, 0x01 };
	headless::checkPacketBytes(ctx, headless::encodePacketData(*nonCanonicalDecoded), canonicalizedTrue,
		"Packet106 true re-encodes as byte one");
}
