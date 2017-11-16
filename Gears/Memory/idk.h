#define STATIC_ASSERT_SIZE(Type, Size) static_assert(sizeof(Type) == Size, "invalid " #Type " size")

#pragma pack(push, 1)

namespace rage
{
	class datBase
	{
	public:
		virtual ~datBase() {}
	};

	class fwArchetype : public datBase
	{
	public:
		virtual ~fwArchetype() {}
		virtual void Initialize() {}
		virtual void InitializeFromArchetypeDef(uint32_t, fwArchetypeDef*, bool) {}
		virtual fwEntity* CreateEntity() { return nullptr; }
		// and lots of other functions...

	public:
		char _0x0008[0x10];			// 0x0008
		Hash m_hash;				// 0x0018
		char _0x001C[0x10];			// 0x001C
		float m_radius;				// 0x002C
		Vec4 m_aabbMin;				// 0x0030
		Vec4 m_aabbMax;				// 0x0040
		eArchetypeFlag m_flags;		// 0x0050
		char _0x0054[0x4];			// 0x0054
		void* m_0x0058;				// 0x0058
		uint8_t m_0x0060;			// 0x0060
		char _0x0061;				// 0x0061
		uint16_t m_assetStoreIndex; // 0x0062
		char _0x0064[0x2];			// 0x0064
		uint16_t m_index;			// 0x0066
	};

	STATIC_ASSERT_SIZE(fwArchetype, 0x68);
}

class CBaseModelInfo : public rage::fwArchetype
{
public:
	virtual ~CBaseModelInfo() {}
	virtual void Initialize() {}
	virtual void InitializeFromArchetypeDef(uint32_t, rage::fwArchetypeDef*, bool) {}
	virtual rage::fwEntity* CreateEntity() { return nullptr; }
	// and lots of other functions...

public:
	eModelType GetModelType() const
	{
		return m_modelType & 0x1F;
	}

protected:
	char _0x0068[0x35];			// 0x0068
	eModelType m_modelType;		// 0x009D (& 0x1F)
	char _0x009E[0x2];			// 0x009E
	uint32_t m_unkFlag;			// 0x00A0
	char _0x00A4[0x4];			// 0x00A4
	void* m_0x00A8;				// 0x00A8
};

STATIC_ASSERT_SIZE(CBaseModelInfo, 0xB0);

class CVehicleModelInfo : public CBaseModelInfo
{
public:
	virtual ~CVehicleModelInfo() {}
	virtual void Initialize() {}
	virtual void InitializeFromArchetypeDef(uint32_t, rage::fwArchetypeDef*, bool) {}
	virtual rage::fwEntity* CreateEntity() { return nullptr; }
	// and lots of other functions...

public:
	void* m_0x00B0; // 0x00B0
	char _0x00B8[0x40]; // 0x00B8
	uint8_t m_primaryColorCombinations[25]; // 0x00F8
	uint8_t m_secondaryColorCombinations[25]; // 0x0111
	uint8_t m_unkColor1Combinations[25]; // 0x012A
	uint8_t m_unkColor2Combinations[25]; // 0x0143
	uint8_t m_interiorColorCombinations[25]; // 0x015C
	uint8_t m_dashboardColorCombinations[25]; // 0x0175
	char _0x018E[0xE2]; // 0x018E
	char m_displayName[12]; // 0x0270 (aka gameName)
	char m_manufacturerName[12]; // 0x027C (aka makeName)
	uint8_t* m_modKits; // 0x0288
	uint16_t m_modKitsCount; // 0x0290
	char _0x0292[0x46]; // 0x0292
	void* m_driverInfo; // 0x02D8
	uint8_t m_numDrivers; // 0x02E0
	char _0x02E1[0x37]; // 0x02E1
	eVehicleType m_vehicleType; // 0x0318
	uint32_t m_unkVehicleType; // 0x031C
	uint32_t m_diffuseTint; // 0x0320
	char _0x0324[0x90]; // 0x0324
	uint8_t m_unkModKitVal; // 0x03B4
	char _0x03B5[0xA7]; // 0x03B5
	float m_wheelScale; // 0x045C
	float m_wheelScaleRear; // 0x0460
	float m_defaultBodyHealth; // 0x0464
	char _0x0468[0x20]; // 0x0468
	uint32_t m_handlingIndex; // 0x0488
	uint32_t m_identicalModelSpawnDistance; // 0x048C
	char _0x0490[0x4]; // 0x0490
	uint32_t m_numColorCombinations; // 0x0494
	char _0x0498[0x30]; // 0x0498
	void* m_0x04C8; // 0x04C8 (wheel data? 0xAC -> burnout mult?)
	char _0x04D0[0x3B]; // 0x04D0
	uint8_t m_sirenInfoId; // 0x050B
	char _0x050C[0xC]; // 0x050C
	uint8_t m_vehicleClass; // 0x0518 (& 0x1F; (>> 5) & 3 -> plate type)
	char _0x0519[0x2F]; // 0x0519
	int m_seatCount; // 0x0548
	eVehicleFlag1 m_flags1; // 0x054C
	eVehicleFlag2 m_flags2; // 0x0550
	eVehicleFlag3 m_flags3; // 0x0554
	eVehicleFlag4 m_flags4; // 0x0558
	eVehicleFlag5 m_flags5; // 0x055C
};

STATIC_ASSERT_SIZE(CVehicleModelInfo, 0x560);

#pragma pack(pop)