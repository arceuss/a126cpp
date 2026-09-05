cmake_minimum_required(VERSION 3.15)

project(Alpha126Cpp
	VERSION 1.2.6.0
	DESCRIPTION "Minecraft Alpha 1.2.6 C++ Port"
	LANGUAGES C CXX
)

# Export compile commands for clangd and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Alpha126Cpp_SOURCES
	"src/client/Minecraft.cpp"
	"src/client/Minecraft.h"
	
	"src/Facing.h"
	"src/SharedConstants.cpp"
	"src/SharedConstants.h"
	
	"src/client/Lighting.cpp"
	"src/client/Lighting.h"
	"src/client/MemoryTracker.cpp"
	"src/client/MemoryTracker.h"
	"src/client/MouseHandler.cpp"
	"src/client/MouseHandler.h"
	"src/client/OpenGLCapabilities.cpp"
	"src/client/OpenGLCapabilities.h"
	"src/client/Options.cpp"
	"src/client/Options.h"
	"src/client/ProgressRenderer.cpp"
	"src/client/ProgressRenderer.h"
	"src/client/Timer.cpp"
	"src/client/Timer.h"
	"src/client/User.cpp"
	"src/client/User.h"

	"src/client/spc/SPCCommand.cpp"
	"src/client/spc/SPCCommand.h"

	"src/client/gamemode/GameMode.cpp"
	"src/client/gamemode/GameMode.h"
	"src/client/gamemode/SurvivalMode.cpp"
	"src/client/gamemode/SurvivalMode.h"
	"src/client/gamemode/MultiplayerMode.cpp"
	"src/client/gamemode/MultiplayerMode.h"
	
	"src/client/gui/Button.cpp"
	"src/client/gui/Button.h"
	"src/client/gui/ConfirmScreen.cpp"
	"src/client/gui/ConfirmScreen.h"
	"src/client/gui/ControlsScreen.cpp"
	"src/client/gui/ControlsScreen.h"
	"src/client/gui/ClientOptionsScreen.cpp"
	"src/client/gui/ClientOptionsScreen.h"
	"src/client/gui/DeathScreen.cpp"
	"src/client/gui/DeathScreen.h"
	"src/client/gui/DeleteWorldScreen.cpp"
	"src/client/gui/DeleteWorldScreen.h"
	"src/client/gui/Font.cpp"
	"src/client/gui/Font.h"
	"src/client/gui/Gui.cpp"
	"src/client/gui/Gui.h"
	"src/client/gui/GuiComponent.cpp"
	"src/client/gui/GuiComponent.h"
	"src/client/gui/InventoryScreen.cpp"
	"src/client/gui/InventoryScreen.h"
	"src/client/gui/WorkbenchScreen.cpp"
	"src/client/gui/WorkbenchScreen.h"
	"src/client/gui/ChestScreen.cpp"
	"src/client/gui/ChestScreen.h"
	"src/client/gui/FurnaceScreen.cpp"
	"src/client/gui/FurnaceScreen.h"
	"src/client/gui/TextEditScreen.cpp"
	"src/client/gui/TextEditScreen.h"
	"src/client/gui/OptionsScreen.cpp"
	"src/client/gui/OptionsScreen.h"
	"src/client/gui/PauseScreen.cpp"
	"src/client/gui/PauseScreen.h"
	"src/client/gui/Screen.cpp"
	"src/client/gui/Screen.h"
	"src/client/gui/ScreenSizeCalculator.cpp"
	"src/client/gui/ScreenSizeCalculator.h"
	"src/client/gui/SelectWorldScreen.cpp"
	"src/client/gui/SelectWorldScreen.h"
	"src/client/gui/SlideButton.cpp"
	"src/client/gui/SlideButton.h"
	"src/client/gui/SmallButton.cpp"
	"src/client/gui/SmallButton.h"
	"src/client/gui/GuiTextField.cpp"
	"src/client/gui/GuiTextField.h"
	"src/client/gui/ChatLine.cpp"
	"src/client/gui/ChatLine.h"
	"src/client/gui/GuiMessage.cpp"
	"src/client/gui/GuiMessage.h"
	"src/client/gui/GuiChat.cpp"
	"src/client/gui/GuiChat.h"
	"src/client/gui/GuiNewChat.cpp"
	"src/client/gui/GuiNewChat.h"
	"src/client/gui/GuiMultiplayer.cpp"
	"src/client/gui/GuiMultiplayer.h"
	"src/client/gui/GuiConnecting.cpp"
	"src/client/gui/GuiConnecting.h"
	"src/client/gui/GuiConnectFailed.cpp"
	"src/client/gui/GuiDownloadTerrain.cpp"
	"src/client/gui/GuiDownloadTerrain.h"
	"src/client/gui/GuiConnectFailed.h"

	"src/client/multiplayer/MultiPlayerLevel.cpp"
	"src/client/multiplayer/MultiPlayerLevel.h"
	"src/client/multiplayer/MultiPlayerChunkCache.cpp"
	"src/client/multiplayer/MultiPlayerChunkCache.h"

	"src/client/locale/Language.cpp"
	"src/client/locale/Language.h"
	
	"src/client/model/Cube.cpp"
	"src/client/model/Cube.h"
	"src/client/model/HumanoidModel.cpp"
	"src/client/model/HumanoidModel.h"
	"src/client/model/Model.cpp"
	"src/client/model/Model.h"
	"src/client/model/Polygon.cpp"
	"src/client/model/Polygon.h"
	"src/client/model/QuadrupedModel.cpp"
	"src/client/model/QuadrupedModel.h"
	"src/client/model/PigModel.cpp"
	"src/client/model/PigModel.h"
	"src/client/model/CowModel.cpp"
	"src/client/model/CowModel.h"
	"src/client/model/SheepModel.cpp"
	"src/client/model/SheepModel.h"
	"src/client/model/SheepFurModel.cpp"
	"src/client/model/SheepFurModel.h"
	"src/client/model/ChickenModel.cpp"
	"src/client/model/ChickenModel.h"
	"src/client/model/SignModel.cpp"
	"src/client/model/SignModel.h"
	"src/client/model/BoatModel.cpp"
	"src/client/model/BoatModel.h"
	"src/client/model/MinecartModel.cpp"
	"src/client/model/MinecartModel.h"
	"src/client/model/CreeperModel.cpp"
	"src/client/model/CreeperModel.h"
	"src/client/model/ZombieModel.cpp"
	"src/client/model/ZombieModel.h"
	"src/client/model/SkeletonModel.cpp"
	"src/client/model/SkeletonModel.h"
	"src/client/model/SpiderModel.cpp"
	"src/client/model/SpiderModel.h"
	"src/client/model/SlimeModel.cpp"
	"src/client/model/SlimeModel.h"
	"src/client/model/GhastModel.cpp"
	"src/client/model/GhastModel.h"
	"src/client/model/Vertex.cpp"
	"src/client/model/Vertex.h"
	
	"src/client/player/Input.cpp"
	"src/client/player/Input.h"
	"src/client/player/KeyboardInput.cpp"
	"src/client/player/KeyboardInput.h"
	"src/client/player/LocalPlayer.cpp"
	"src/client/player/LocalPlayer.h"
	"src/client/player/EntityClientPlayerMP.cpp"
	"src/client/player/EntityClientPlayerMP.h"
	
	"src/client/renderer/Chunk.cpp"
	"src/client/renderer/Chunk.h"
	"src/client/renderer/DirtyChunkSorter.h"
	"src/client/renderer/DistanceChunkSorter.h"
	"src/client/renderer/GameRenderer.cpp"
	"src/client/renderer/GameRenderer.h"
	"src/client/renderer/ItemInHandRenderer.cpp"
	"src/client/renderer/ItemInHandRenderer.h"
	"src/client/renderer/LevelRenderer.cpp"
	"src/client/renderer/LevelRenderer.h"
	"src/client/renderer/OffsettedRenderList.cpp"
	"src/client/renderer/OffsettedRenderList.h"
	"src/client/renderer/Tesselator.cpp"
	"src/client/renderer/Tesselator.h"
	"src/client/renderer/Textures.cpp"
	"src/client/renderer/Textures.h"
	"src/client/renderer/HttpTexture.cpp"
	"src/client/renderer/HttpTexture.h"
	"src/client/renderer/HttpTextureProcessor.h"
	"src/client/renderer/MobSkinTextureProcessor.cpp"
	"src/client/renderer/MobSkinTextureProcessor.h"
	"src/client/renderer/TextureFX.cpp"
	"src/client/renderer/TextureFX.h"
	"src/client/renderer/TextureWaterFX.cpp"
	"src/client/renderer/TextureWaterFX.h"
	"src/client/renderer/TextureWaterSideFX.cpp"
	"src/client/renderer/TextureWaterSideFX.h"
	"src/client/renderer/TextureLavaFX.cpp"
	"src/client/renderer/TextureLavaFX.h"
	"src/client/renderer/TextureLavaSideFX.cpp"
	"src/client/renderer/TextureLavaSideFX.h"
	"src/client/renderer/TextureFireFX.cpp"
	"src/client/renderer/TextureFireFX.h"
	"src/client/renderer/TexturePortalFX.cpp"
	"src/client/renderer/TexturePortalFX.h"
	"src/client/renderer/TextureCompassFX.cpp"
	"src/client/renderer/TextureCompassFX.h"
	"src/client/renderer/TextureClockFX.cpp"
	"src/client/renderer/TextureClockFX.h"
	"src/client/renderer/TileRenderer.cpp"
	"src/client/renderer/TileRenderer.h"

	"src/client/renderer/culling/Culler.h"
	"src/client/renderer/culling/Frustum.cpp"
	"src/client/renderer/culling/Frustum.h"
	"src/client/renderer/culling/FrustumCuller.cpp"
	"src/client/renderer/culling/FrustumCuller.h"
	"src/client/renderer/culling/FrustumData.cpp"
	"src/client/renderer/culling/OcclusionCullState.h"
	"src/client/renderer/culling/OcclusionCuller.cpp"
	"src/client/renderer/culling/OcclusionCuller.h"
	"src/client/renderer/culling/FrustumData.h"
	
	"src/client/renderer/entity/EntityRenderDispatcher.cpp"
	"src/client/renderer/entity/EntityRenderDispatcher.h"
	"src/client/renderer/entity/EntityRenderer.cpp"
	"src/client/renderer/entity/EntityRenderer.h"
	"src/client/renderer/entity/ItemRenderer.cpp"
	"src/client/renderer/entity/ItemRenderer.h"
	"src/client/renderer/entity/MobRenderer.cpp"
	"src/client/renderer/entity/MobRenderer.h"
	"src/client/renderer/entity/PlayerRenderer.cpp"
	"src/client/renderer/entity/PlayerRenderer.h"
	"src/client/renderer/entity/PigRenderer.cpp"
	"src/client/renderer/entity/PigRenderer.h"
	"src/client/renderer/entity/CowRenderer.cpp"
	"src/client/renderer/entity/CowRenderer.h"
	"src/client/renderer/entity/ChickenRenderer.cpp"
	"src/client/renderer/entity/ChickenRenderer.h"
	"src/client/renderer/entity/SheepRenderer.cpp"
	"src/client/renderer/entity/SheepRenderer.h"
	"src/client/renderer/entity/FallingTileRenderer.cpp"
	"src/client/renderer/entity/FallingTileRenderer.h"
	"src/client/renderer/entity/BoatRenderer.cpp"
	"src/client/renderer/entity/BoatRenderer.h"
	"src/client/renderer/entity/MinecartRenderer.cpp"
	"src/client/renderer/entity/MinecartRenderer.h"
	"src/client/renderer/entity/PaintingRenderer.cpp"
	"src/client/renderer/entity/PaintingRenderer.h"
	"src/client/renderer/entity/TntRenderer.cpp"
	"src/client/renderer/entity/TntRenderer.h"
	"src/client/renderer/entity/ArrowRenderer.cpp"
	"src/client/renderer/entity/ArrowRenderer.h"
	"src/client/renderer/entity/FishingHookRenderer.cpp"
	"src/client/renderer/entity/FishingHookRenderer.h"
	"src/client/renderer/entity/CreeperRenderer.cpp"
	"src/client/renderer/entity/CreeperRenderer.h"
	"src/client/renderer/entity/HumanoidMobRenderer.cpp"
	"src/client/renderer/entity/HumanoidMobRenderer.h"
	"src/client/renderer/entity/GiantMobRenderer.cpp"
	"src/client/renderer/entity/GiantMobRenderer.h"
	"src/client/renderer/entity/SlimeRenderer.cpp"
	"src/client/renderer/entity/SlimeRenderer.h"
	"src/client/renderer/entity/GhastRenderer.cpp"
	"src/client/renderer/entity/GhastRenderer.h"
	"src/client/renderer/entity/FireballRenderer.cpp"
	"src/client/renderer/entity/FireballRenderer.h"
	"src/client/renderer/entity/SpiderRenderer.cpp"
	"src/client/renderer/entity/SpiderRenderer.h"
	
	"src/client/renderer/tileentity/TileEntityRenderDispatcher.cpp"
	"src/client/renderer/tileentity/TileEntityRenderDispatcher.h"
	"src/client/renderer/tileentity/TileEntityRenderer.cpp"
	"src/client/renderer/tileentity/TileEntityRenderer.h"
	"src/client/renderer/tileentity/SignRenderer.cpp"
	"src/client/renderer/tileentity/SignRenderer.h"

	"src/client/skins/DefaultTexturePack.cpp"
	"src/client/skins/DefaultTexturePack.h"
	"src/client/skins/FileTexturePack.cpp"
	"src/client/skins/FileTexturePack.h"
	"src/client/skins/TexturePack.cpp"
	"src/client/skins/TexturePack.h"
	"src/client/skins/TexturePackRepository.cpp"
	"src/client/skins/TexturePackRepository.h"
	"src/client/skins/TexturePackSelectScreen.cpp"
	"src/client/skins/TexturePackSelectScreen.h"

	"src/client/sound/SoundEngine.cpp"
	"src/client/sound/SoundEngine.h"
	"src/client/sound/SoundRepository.cpp"
	"src/client/sound/SoundRepository.h"
	
	"src/client/particle/Particle.cpp"
	"src/client/particle/Particle.h"
	"src/client/particle/WaterDropParticle.cpp"
	"src/client/particle/WaterDropParticle.h"
	"src/client/particle/SplashParticle.cpp"
	"src/client/particle/SplashParticle.h"
	"src/client/particle/BubbleParticle.cpp"
	"src/client/particle/BubbleParticle.h"
	"src/client/particle/SmokeParticle.cpp"
	"src/client/particle/SmokeParticle.h"
	"src/client/particle/FlameParticle.cpp"
	"src/client/particle/FlameParticle.h"
	"src/client/particle/LavaParticle.cpp"
	"src/client/particle/LavaParticle.h"
	"src/client/particle/ExplodeParticle.cpp"
	"src/client/particle/ExplodeParticle.h"
	"src/client/particle/RedDustParticle.cpp"
	"src/client/particle/RedDustParticle.h"
	"src/client/particle/PortalParticle.cpp"
	"src/client/particle/PortalParticle.h"
	"src/client/particle/TakeAnimationParticle.cpp"
	"src/client/particle/TakeAnimationParticle.h"
	"src/client/particle/TerrainParticle.cpp"
	"src/client/particle/TerrainParticle.h"
	"src/client/particle/ParticleEngine.cpp"
	"src/client/particle/ParticleEngine.h"
	"src/client/sound/Sound.h"

	"src/client/title/TitleScreen.cpp"
	"src/client/title/TitleScreen.h"

	"src/locale/Descriptive.h"
	
	"src/nbt/ByteArrayTag.cpp"
	"src/nbt/ByteArrayTag.h"
	"src/nbt/ByteTag.cpp"
	"src/nbt/ByteTag.h"
	"src/nbt/CompoundTag.cpp"
	"src/nbt/CompoundTag.h"
	"src/nbt/DoubleTag.cpp"
	"src/nbt/DoubleTag.h"
	"src/nbt/EndTag.cpp"
	"src/nbt/EndTag.h"
	"src/nbt/FloatTag.cpp"
	"src/nbt/FloatTag.h"
	"src/nbt/IntTag.cpp"
	"src/nbt/IntTag.h"
	"src/nbt/ListTag.cpp"
	"src/nbt/ListTag.h"
	"src/nbt/LongTag.cpp"
	"src/nbt/LongTag.h"
	"src/nbt/NbtIo.cpp"
	"src/nbt/NbtIo.h"
	"src/nbt/ShortTag.cpp"
	"src/nbt/ShortTag.h"
	"src/nbt/StringTag.cpp"
	"src/nbt/StringTag.h"
	"src/nbt/Tag.cpp"
	"src/nbt/Tag.h"

	"src/util/BackgroundTask.cpp"
	"src/util/BackgroundTask.h"
	"src/tools/MemoryProbe.cpp"
	"src/tools/MemoryProbe.h"
	"src/util/GLU.cpp"
	"src/util/GLU.h"
	"src/util/HttpGet.cpp"
	"src/util/HttpGet.h"
	"src/util/IntHashMap.h"
	"src/util/Memory.h"
	"src/util/Mth.cpp"
	"src/util/Mth.h"
	"src/util/musDecode.h"
	"src/util/ProgressListener.h"
	"src/util/BlockDropTest.cpp"
	"src/util/BlockDropTest.h"

	"src/world/entity/Entity.cpp"
	"src/world/entity/Entity.h"
	"src/world/entity/EntityEvent.h"
	"src/world/entity/EntityIO.cpp"
	"src/world/entity/EntityIO.h"
	"src/world/entity/EntityPos.cpp"
	"src/world/entity/EntityPos.h"
	"src/world/entity/Mob.cpp"
	"src/world/entity/Mob.h"
	"src/world/entity/DataWatcher.cpp"
	"src/world/entity/DataWatcher.h"
	"src/world/entity/WatchableObject.h"
	"src/world/entity/FlyingMob.cpp"
	"src/world/entity/FlyingMob.h"
	"src/world/entity/MobCategory.cpp"
	"src/world/entity/MobCategory.h"

	"src/world/entity/monster/PathfinderMob.cpp"
	"src/world/entity/monster/PathfinderMob.h"
	"src/world/entity/monster/Monster.cpp"
	"src/world/entity/monster/Monster.h"
	"src/world/entity/monster/Creeper.cpp"
	"src/world/entity/monster/Creeper.h"
	"src/world/entity/monster/Skeleton.cpp"
	"src/world/entity/monster/Skeleton.h"
	"src/world/entity/monster/Spider.cpp"
	"src/world/entity/monster/Spider.h"
	"src/world/entity/monster/Giant.cpp"
	"src/world/entity/monster/Giant.h"
	"src/world/entity/monster/Zombie.cpp"
	"src/world/entity/monster/Zombie.h"
	"src/world/entity/monster/Slime.cpp"
	"src/world/entity/monster/Slime.h"
	"src/world/entity/monster/Ghast.cpp"
	"src/world/entity/monster/Ghast.h"
	"src/world/entity/monster/PigZombie.cpp"
	"src/world/entity/monster/PigZombie.h"

	"src/world/entity/animal/Animal.cpp"
	"src/world/entity/animal/Animal.h"
	"src/world/entity/animal/Chicken.cpp"
	"src/world/entity/animal/Chicken.h"
	"src/world/entity/animal/Cow.cpp"
	"src/world/entity/animal/Cow.h"
	"src/world/entity/animal/Pig.cpp"
	"src/world/entity/animal/Pig.h"
	"src/world/entity/animal/Sheep.cpp"
	"src/world/entity/animal/Sheep.h"

	"src/world/entity/player/Player.cpp"
	"src/world/entity/player/Player.h"
	"src/world/entity/player/EntityOtherPlayerMP.cpp"
	"src/world/entity/player/EntityOtherPlayerMP.h"
	"src/world/entity/player/InventoryPlayer.cpp"
	"src/world/entity/player/InventoryPlayer.h"

	"src/world/entity/item/EntityItem.cpp"
	"src/world/entity/item/EntityItem.h"

	"src/world/entity/projectile/Arrow.cpp"
	"src/world/entity/projectile/Arrow.h"
	"src/world/entity/projectile/Snowball.cpp"
	"src/world/entity/projectile/Snowball.h"
	"src/world/entity/projectile/Fireball.cpp"
	"src/world/entity/projectile/Fireball.h"
	"src/world/entity/projectile/FishingHook.cpp"
	"src/world/entity/projectile/FishingHook.h"

	"src/world/entity/Painting.cpp"
	"src/world/entity/Painting.h"
	"src/world/entity/PrimedTnt.cpp"
	"src/world/entity/PrimedTnt.h"
	"src/world/entity/item/FallingTile.cpp"
	"src/world/entity/item/FallingTile.h"
	"src/world/entity/item/Boat.cpp"
	"src/world/entity/item/Boat.h"
	"src/world/entity/item/Minecart.cpp"
	"src/world/entity/item/Minecart.h"

	"src/world/level/Explosion.cpp"
	"src/world/level/Explosion.h"

	"src/world/CompoundContainer.cpp"
	"src/world/CompoundContainer.h"

	"src/world/item/ItemInstance.cpp"
	"src/world/item/ItemInstance.h"
	"src/world/item/Item.cpp"
	"src/world/item/Item.h"
	"src/world/item/ItemStack.cpp"
	"src/world/item/ItemStack.h"
	"src/world/item/ItemTool.cpp"
	"src/world/item/ItemTool.h"
	"src/world/item/ItemPickaxe.cpp"
	"src/world/item/ItemPickaxe.h"
	"src/world/item/ItemSpade.cpp"
	"src/world/item/ItemSpade.h"
	"src/world/item/ItemAxe.cpp"
	"src/world/item/ItemAxe.h"
	"src/world/item/ItemSword.cpp"
	"src/world/item/ItemSword.h"
	"src/world/item/ItemFood.cpp"
	"src/world/item/ItemFood.h"
	"src/world/item/ItemSoup.cpp"
	"src/world/item/ItemSoup.h"
	"src/world/item/ItemFlintAndSteel.cpp"
	"src/world/item/ItemFlintAndSteel.h"
	"src/world/item/ItemBow.cpp"
	"src/world/item/ItemBow.h"
	"src/world/item/ItemHoe.cpp"
	"src/world/item/ItemHoe.h"
	"src/world/item/ItemSeeds.cpp"
	"src/world/item/ItemSeeds.h"
	"src/world/item/ItemArmor.cpp"
	"src/world/item/ItemArmor.h"
	"src/world/item/ItemPainting.cpp"
	"src/world/item/ItemPainting.h"
	"src/world/item/ItemSign.cpp"
	"src/world/item/ItemSign.h"
	"src/world/item/ItemDoor.cpp"
	"src/world/item/ItemDoor.h"
	"src/world/item/ItemBucket.cpp"
	"src/world/item/ItemBucket.h"
	"src/world/item/ItemMinecart.cpp"
	"src/world/item/ItemMinecart.h"
	"src/world/item/ItemSaddle.cpp"
	"src/world/item/ItemSaddle.h"
	"src/world/item/ItemRedstone.cpp"
	"src/world/item/ItemRedstone.h"
	"src/world/item/ItemSnowball.cpp"
	"src/world/item/ItemSnowball.h"
	"src/world/item/ItemBoat.cpp"
	"src/world/item/ItemBoat.h"
	"src/world/item/ItemReed.cpp"
	"src/world/item/ItemReed.h"
	"src/world/item/ItemFishingRod.cpp"
	"src/world/item/ItemFishingRod.h"
	"src/world/item/ItemRecord.cpp"
	"src/world/item/ItemRecord.h"
	"src/world/item/Items.cpp"
	"src/world/item/Items.h"
	
	"src/world/item/crafting/Recipy.h"
	"src/world/item/crafting/ShapedRecipy.cpp"
	"src/world/item/crafting/ShapedRecipy.h"
	"src/world/item/crafting/ShapelessRecipy.cpp"
	"src/world/item/crafting/ShapelessRecipy.h"
	"src/world/item/crafting/CraftingContainer.h"
	"src/world/item/crafting/Recipes.cpp"
	"src/world/item/crafting/Recipes.h"
	"src/world/item/crafting/ToolRecipies.cpp"
	"src/world/item/crafting/ToolRecipies.h"
	"src/world/item/crafting/WeaponRecipies.cpp"
	"src/world/item/crafting/WeaponRecipies.h"
	"src/world/item/crafting/OreRecipies.cpp"
	"src/world/item/crafting/OreRecipies.h"
	"src/world/item/crafting/FoodRecipies.cpp"
	"src/world/item/crafting/FoodRecipies.h"
	"src/world/item/crafting/StructureRecipies.cpp"
	"src/world/item/crafting/StructureRecipies.h"
	"src/world/item/crafting/ArmorRecipes.cpp"
	"src/world/item/crafting/ArmorRecipes.h"
	"src/world/item/crafting/FurnaceRecipes.cpp"
	"src/world/item/crafting/FurnaceRecipes.h"
	
	"src/world/level/ChunkPos.cpp"
	"src/world/level/ChunkPos.h"
	"src/world/level/Coord.h"
	"src/world/level/FoliageColor.cpp"
	"src/world/level/FoliageColor.h"
	"src/world/level/GrassColor.cpp"
	"src/world/level/GrassColor.h"
	"src/world/level/Level.cpp"
	"src/world/level/Level.h"
	"src/world/level/LevelListener.h"
	"src/world/level/LevelSource.h"
	"src/world/level/LightLayer.h"
	"src/world/level/LightUpdate.cpp"
	"src/world/level/LightUpdate.h"
	"src/world/level/NextTickListEntry.cpp"
	"src/world/level/NextTickListEntry.h"
	"src/world/level/Region.cpp"
	"src/world/level/Region.h"
	"src/world/level/Teleporter.cpp"
	"src/world/level/Teleporter.h"
	"src/world/level/TilePos.h"
	"src/world/level/MobSpawner.cpp"
	"src/world/level/MobSpawner.h"
	
	"src/world/level/pathfinder/BinaryHeap.h"
	"src/world/level/pathfinder/Node.cpp"
	"src/world/level/pathfinder/Node.h"
	"src/world/level/pathfinder/Path.cpp"
	"src/world/level/pathfinder/Path.h"
	"src/world/level/pathfinder/PathFinder.cpp"
	"src/world/level/pathfinder/PathFinder.h"

	"src/world/level/biome/BiomeSource.cpp"
	"src/world/level/biome/BiomeSource.h"
	"src/world/level/biome/FixedBiomeSource.cpp"
	"src/world/level/biome/FixedBiomeSource.h"
	"src/world/level/biome/BiomeMobs.cpp"
	"src/world/level/biome/BiomeMobs.h"

	"src/world/level/chunk/storage/OldChunkStorage.cpp"
	"src/world/level/chunk/storage/OldChunkStorage.h"
	"src/world/level/chunk/storage/ChunkStorage.h"
	"src/world/level/chunk/storage/RegionFile.cpp"
	"src/world/level/chunk/storage/RegionFile.h"
	"src/world/level/chunk/storage/RegionFileCache.cpp"
	"src/world/level/chunk/storage/RegionFileCache.h"
	"src/world/level/chunk/storage/McRegionChunkStorage.cpp"
	"src/world/level/chunk/storage/McRegionChunkStorage.h"
	"src/world/level/SaveConverterMcRegion.cpp"
	"src/world/level/SaveConverterMcRegion.h"
	
	"src/world/level/chunk/ChunkCache.cpp"
	"src/world/level/chunk/ChunkCache.h"
	"src/world/level/chunk/ChunkSource.h"
	"src/world/level/chunk/DataLayer.cpp"
	"src/world/level/chunk/DataLayer.h"
	"src/world/level/chunk/EmptyLevelChunk.cpp"
	"src/world/level/chunk/EmptyLevelChunk.h"
	"src/world/level/chunk/LevelChunk.cpp"
	"src/world/level/chunk/LevelChunk.h"

	"src/world/level/dimension/Dimension.cpp"
	"src/world/level/dimension/Dimension.h"
	"src/world/level/dimension/HellDimension.cpp"
	"src/world/level/dimension/HellDimension.h"
	
	"src/world/level/levelgen/feature/CaveFeature.cpp"
	"src/world/level/levelgen/feature/CaveFeature.h"
	"src/world/level/levelgen/feature/Feature.h"
	"src/world/level/levelgen/feature/TreeFeature.cpp"
	"src/world/level/levelgen/feature/TreeFeature.h"
	"src/world/level/levelgen/feature/WorldGenerator.h"
	"src/world/level/levelgen/feature/WorldGenFlowers.cpp"
	"src/world/level/levelgen/feature/WorldGenFlowers.h"
	"src/world/level/levelgen/feature/WorldGenReed.cpp"
	"src/world/level/levelgen/feature/WorldGenReed.h"
	"src/world/level/levelgen/feature/WorldGenCactus.cpp"
	"src/world/level/levelgen/feature/WorldGenCactus.h"
	"src/world/level/levelgen/feature/WorldGenMinable.cpp"
	"src/world/level/levelgen/feature/WorldGenMinable.h"
	"src/world/level/levelgen/feature/WorldGenLakes.cpp"
	"src/world/level/levelgen/feature/WorldGenLakes.h"
	"src/world/level/levelgen/feature/WorldGenLiquids.cpp"
	"src/world/level/levelgen/feature/WorldGenLiquids.h"
	"src/world/level/levelgen/feature/MonsterRoomFeature.cpp"
	"src/world/level/levelgen/feature/MonsterRoomFeature.h"
	"src/world/level/levelgen/feature/WorldGenBigTree.cpp"
	"src/world/level/levelgen/feature/WorldGenBigTree.h"
	"src/world/level/levelgen/feature/PumpkinFeature.cpp"
	"src/world/level/levelgen/feature/PumpkinFeature.h"
	"src/world/level/levelgen/feature/OreFeature.cpp"
	"src/world/level/levelgen/feature/OreFeature.h"
	"src/world/level/levelgen/feature/WorldGenHellLava.cpp"
	"src/world/level/levelgen/feature/WorldGenHellLava.h"
	"src/world/level/levelgen/feature/WorldGenFire.cpp"
	"src/world/level/levelgen/feature/WorldGenFire.h"
	"src/world/level/levelgen/feature/WorldGenLightStone1.cpp"
	"src/world/level/levelgen/feature/WorldGenLightStone1.h"
	"src/world/level/levelgen/feature/WorldGenLightStone2.cpp"
	"src/world/level/levelgen/feature/WorldGenLightStone2.h"
	
	"src/world/level/levelgen/synth/ImprovedNoise.cpp"
	"src/world/level/levelgen/synth/ImprovedNoise.h"
	"src/world/level/levelgen/synth/PerlinNoise.cpp"
	"src/world/level/levelgen/synth/PerlinNoise.h"
	"src/world/level/levelgen/synth/PerlinSimplexNoise.cpp"
	"src/world/level/levelgen/synth/PerlinSimplexNoise.h"
	"src/world/level/levelgen/synth/SimplexNoise.cpp"
	"src/world/level/levelgen/synth/SimplexNoise.h"
	"src/world/level/levelgen/synth/Synth.cpp"
	"src/world/level/levelgen/synth/Synth.h"
	
	"src/world/level/levelgen/LargeCaveFeature.cpp"
	"src/world/level/levelgen/LargeCaveFeature.h"
	"src/world/level/levelgen/LargeFeature.cpp"
	"src/world/level/levelgen/LargeFeature.h"
	"src/world/level/levelgen/RandomLevelSource.cpp"
	"src/world/level/levelgen/RandomLevelSource.h"
	"src/world/level/levelgen/HellCaveFeature.cpp"
	"src/world/level/levelgen/HellCaveFeature.h"
	"src/world/level/levelgen/HellRandomLevelSource.cpp"
	"src/world/level/levelgen/HellRandomLevelSource.h"
	
	"src/world/level/material/DecorationMaterial.cpp"
	"src/world/level/material/DecorationMaterial.h"
	"src/world/level/material/GasMaterial.cpp"
	"src/world/level/material/GasMaterial.h"
	"src/world/level/material/LiquidMaterial.cpp"
	"src/world/level/material/LiquidMaterial.h"
	"src/world/level/material/Material.cpp"
	"src/world/level/material/Material.h"
	"src/world/level/material/PlantMaterial.cpp"
	"src/world/level/material/PlantMaterial.h"

	"src/world/level/tile/entity/TileEntity.cpp"
	"src/world/level/tile/entity/TileEntity.h"
	"src/world/level/tile/entity/ChestTileEntity.cpp"
	"src/world/level/tile/entity/ChestTileEntity.h"
	"src/world/level/tile/entity/MobSpawnerTileEntity.cpp"
	"src/world/level/tile/entity/MobSpawnerTileEntity.h"
	"src/world/level/tile/entity/FurnaceTileEntity.cpp"
	"src/world/level/tile/entity/FurnaceTileEntity.h"
	"src/world/level/tile/entity/SignTileEntity.cpp"
	"src/world/level/tile/entity/SignTileEntity.h"
	"src/world/level/tile/entity/InventoryBasic.cpp"
	"src/world/level/tile/entity/InventoryBasic.h"
	
	"src/world/level/tile/DirtTile.cpp"
	"src/world/level/tile/DirtTile.h"
	"src/world/level/tile/GrassTile.cpp"
	"src/world/level/tile/GrassTile.h"
	"src/world/level/tile/GravelTile.cpp"
	"src/world/level/tile/GravelTile.h"
	"src/world/level/tile/LeafTile.cpp"
	"src/world/level/tile/LeafTile.h"
	"src/world/level/tile/SandTile.cpp"
	"src/world/level/tile/SandTile.h"
	"src/world/level/tile/StoneTile.cpp"
	"src/world/level/tile/StoneTile.h"
	"src/world/level/tile/CobblestoneTile.cpp"
	"src/world/level/tile/CobblestoneTile.h"
	"src/world/level/tile/WoodTile.cpp"
	"src/world/level/tile/WoodTile.h"
	"src/world/level/tile/UnbreakableTile.cpp"
	"src/world/level/tile/UnbreakableTile.h"
	"src/world/level/tile/ObsidianTile.cpp"
	"src/world/level/tile/ObsidianTile.h"
	"src/world/level/tile/RedBrickTile.cpp"
	"src/world/level/tile/RedBrickTile.h"
	"src/world/level/tile/Tile.cpp"
	"src/world/level/tile/Tile.h"
	"src/world/level/tile/TransparentTile.cpp"
	"src/world/level/tile/TransparentTile.h"
	"src/world/level/tile/TreeTile.cpp"
	"src/world/level/tile/TreeTile.h"
	"src/world/level/tile/FlowerTile.cpp"
	"src/world/level/tile/FlowerTile.h"
	"src/world/level/tile/SaplingTile.cpp"
	"src/world/level/tile/SaplingTile.h"
	"src/world/level/tile/MushroomTile.cpp"
	"src/world/level/tile/MushroomTile.h"
	"src/world/level/tile/ReedTile.cpp"
	"src/world/level/tile/ReedTile.h"
	"src/world/level/tile/CactusTile.cpp"
	"src/world/level/tile/CactusTile.h"
	"src/world/level/tile/ClayTile.cpp"
	"src/world/level/tile/ClayTile.h"
	"src/world/level/tile/CropTile.cpp"
	"src/world/level/tile/CropTile.h"
	"src/world/level/tile/FarmTile.cpp"
	"src/world/level/tile/FarmTile.h"
	"src/world/level/tile/FurnaceTile.cpp"
	"src/world/level/tile/FurnaceTile.h"
	"src/world/level/tile/LadderTile.cpp"
	"src/world/level/tile/LadderTile.h"
	"src/world/level/tile/SignTile.cpp"
	"src/world/level/tile/SignTile.h"
	"src/world/level/tile/RailTile.cpp"
	"src/world/level/tile/RailTile.h"
	"src/world/level/tile/FenceTile.cpp"
	"src/world/level/tile/FenceTile.h"
	"src/world/level/tile/ButtonTile.cpp"
	"src/world/level/tile/ButtonTile.h"
	"src/world/level/tile/LeverTile.cpp"
	"src/world/level/tile/LeverTile.h"
	"src/world/level/tile/DoorTile.cpp"
	"src/world/level/tile/DoorTile.h"
	"src/world/level/tile/RedStoneDustTile.cpp"
	"src/world/level/tile/RedStoneDustTile.h"
	"src/world/level/tile/RedStoneOreTile.cpp"
	"src/world/level/tile/RedStoneOreTile.h"
	"src/world/level/tile/NotGateTile.cpp"
	"src/world/level/tile/NotGateTile.h"
	"src/world/level/tile/PressurePlateTile.cpp"
	"src/world/level/tile/PressurePlateTile.h"
	"src/world/level/tile/IceTile.cpp"
	"src/world/level/tile/IceTile.h"
	"src/world/level/tile/MossyCobblestoneTile.cpp"
	"src/world/level/tile/MossyCobblestoneTile.h"
	"src/world/level/tile/ChestTile.cpp"
	"src/world/level/tile/ChestTile.h"
	"src/world/level/tile/MobSpawnerTile.cpp"
	"src/world/level/tile/MobSpawnerTile.h"
	"src/world/level/tile/SnowTile.cpp"
	"src/world/level/tile/SnowTile.h"
	"src/world/level/tile/SnowBlockTile.cpp"
	"src/world/level/tile/SnowBlockTile.h"
	"src/world/level/tile/TorchTile.cpp"
	"src/world/level/tile/TorchTile.h"
	"src/world/level/tile/FireTile.cpp"
	"src/world/level/tile/FireTile.h"
	"src/world/level/tile/WorkbenchTile.cpp"
	"src/world/level/tile/WorkbenchTile.h"
	"src/world/level/tile/StoneSlabTile.cpp"
	"src/world/level/tile/StoneSlabTile.h"
	"src/world/level/tile/StairTile.cpp"
	"src/world/level/tile/StairTile.h"
	"src/world/level/tile/MetalTile.cpp"
	"src/world/level/tile/MetalTile.h"
	"src/world/level/tile/GlassTile.cpp"
	"src/world/level/tile/GlassTile.h"
	"src/world/level/tile/ClothTile.cpp"
	"src/world/level/tile/ClothTile.h"
	"src/world/level/tile/SpongeTile.cpp"
	"src/world/level/tile/SpongeTile.h"
	"src/world/level/tile/BookshelfTile.cpp"
	"src/world/level/tile/BookshelfTile.h"
	"src/world/level/tile/TntTile.cpp"
	"src/world/level/tile/TntTile.h"
	"src/world/level/tile/OreTile.cpp"
	"src/world/level/tile/OreTile.h"
	"src/world/level/tile/FluidTile.cpp"
	"src/world/level/tile/FluidTile.h"
	"src/world/level/tile/FluidFlowingTile.cpp"
	"src/world/level/tile/FluidFlowingTile.h"
	"src/world/level/tile/FluidStationaryTile.cpp"
	"src/world/level/tile/FluidStationaryTile.h"
	"src/world/level/tile/RecordPlayerTile.cpp"
	"src/world/level/tile/RecordPlayerTile.h"
	"src/world/level/tile/PumpkinTile.cpp"
	"src/world/level/tile/PumpkinTile.h"
	"src/world/level/tile/HellStoneTile.cpp"
	"src/world/level/tile/HellStoneTile.h"
	"src/world/level/tile/HellSandTile.cpp"
	"src/world/level/tile/HellSandTile.h"
	"src/world/level/tile/LightGemTile.cpp"
	"src/world/level/tile/LightGemTile.h"
	"src/world/level/tile/PortalTile.cpp"
	"src/world/level/tile/PortalTile.h"
	
	"src/world/phys/AABB.cpp"
	"src/world/phys/AABB.h"
	"src/world/phys/ChunkCoordinates.h"
	"src/world/phys/HitResult.cpp"
	"src/world/phys/HitResult.h"
	"src/world/phys/Vec3.cpp"
	"src/world/phys/Vec3.h"
	
	"src/network/Packet.cpp"
	"src/network/Packet.h"
	"src/network/Packet0KeepAlive.cpp"
	"src/network/Packet0KeepAlive.h"
	"src/network/Packet1Login.cpp"
	"src/network/Packet1Login.h"
	"src/network/Packet2Handshake.cpp"
	"src/network/Packet2Handshake.h"
	"src/network/Packet3Chat.cpp"
	"src/network/Packet3Chat.h"
	"src/network/Packet4UpdateTime.cpp"
	"src/network/Packet4UpdateTime.h"
	"src/network/Packet5PlayerInventory.cpp"
	"src/network/Packet5PlayerInventory.h"
	"src/network/Packet6SpawnPosition.cpp"
	"src/network/Packet6SpawnPosition.h"
	"src/network/Packet7.cpp"
	"src/network/Packet7.h"
	"src/network/Packet8.cpp"
	"src/network/Packet8.h"
	"src/network/Packet9.cpp"
	"src/network/Packet9.h"
	"src/network/Packet10Flying.cpp"
	"src/network/Packet10Flying.h"
	"src/network/Packet11PlayerPosition.cpp"
	"src/network/Packet11PlayerPosition.h"
	"src/network/Packet12PlayerLook.cpp"
	"src/network/Packet12PlayerLook.h"
	"src/network/Packet13PlayerLookMove.cpp"
	"src/network/Packet13PlayerLookMove.h"
	"src/network/Packet14BlockDig.cpp"
	"src/network/Packet14BlockDig.h"
	"src/network/Packet15Place.cpp"
	"src/network/Packet15Place.h"
	"src/network/Packet16BlockItemSwitch.cpp"
	"src/network/Packet16BlockItemSwitch.h"
	"src/network/Packet17Sleep.cpp"
	"src/network/Packet17Sleep.h"
	"src/network/Packet18ArmAnimation.cpp"
	"src/network/Packet18ArmAnimation.h"
	"src/network/Packet19EntityAction.cpp"
	"src/network/Packet19EntityAction.h"
	"src/network/Packet20NamedEntitySpawn.cpp"
	"src/network/Packet20NamedEntitySpawn.h"
	"src/network/Packet21PickupSpawn.cpp"
	"src/network/Packet21PickupSpawn.h"
	"src/network/Packet22Collect.cpp"
	"src/network/Packet22Collect.h"
	"src/network/Packet23VehicleSpawn.cpp"
	"src/network/Packet23VehicleSpawn.h"
	"src/network/Packet24MobSpawn.cpp"
	"src/network/Packet24MobSpawn.h"
	"src/network/Packet25EntityPainting.cpp"
	"src/network/Packet25EntityPainting.h"
	"src/network/Packet27Position.cpp"
	"src/network/Packet27Position.h"
	"src/network/Packet28.cpp"
	"src/network/Packet28.h"
	"src/network/Packet29DestroyEntity.cpp"
	"src/network/Packet29DestroyEntity.h"
	"src/network/Packet30Entity.cpp"
	"src/network/Packet30Entity.h"
	"src/network/Packet31RelEntityMove.cpp"
	"src/network/Packet31RelEntityMove.h"
	"src/network/Packet32EntityLook.cpp"
	"src/network/Packet32EntityLook.h"
	"src/network/Packet33RelEntityMoveLook.cpp"
	"src/network/Packet33RelEntityMoveLook.h"
	"src/network/Packet34EntityTeleport.cpp"
	"src/network/Packet34EntityTeleport.h"
	"src/network/Packet38.cpp"
	"src/network/Packet38.h"
	"src/network/Packet39.cpp"
	"src/network/Packet39.h"
	"src/network/Packet40EntityMetadata.cpp"
	"src/network/Packet40EntityMetadata.h"
	"src/network/Packet50PreChunk.cpp"
	"src/network/Packet50PreChunk.h"
	"src/network/Packet51MapChunk.cpp"
	"src/network/Packet51MapChunk.h"
	"src/network/Packet52MultiBlockChange.cpp"
	"src/network/Packet52MultiBlockChange.h"
	"src/network/Packet53BlockChange.cpp"
	"src/network/Packet53BlockChange.h"
	"src/network/Packet54PlayNoteBlock.cpp"
	"src/network/Packet54PlayNoteBlock.h"
	# "src/network/Packet59ComplexEntity.cpp"  # TODO: Not yet ported
	# "src/network/Packet59ComplexEntity.h"
	"src/network/Packet60.cpp"
	"src/network/Packet60.h"
	"src/network/Packet61DoorChange.cpp"
	"src/network/Packet61DoorChange.h"
	"src/network/Packet62Sound.cpp"
	"src/network/Packet62Sound.h"
	"src/network/Packet63Digging.cpp"
	"src/network/Packet63Digging.h"
	"src/network/Packet70Bed.cpp"
	"src/network/Packet70Bed.h"
	"src/network/Packet71Weather.cpp"
	"src/network/Packet71Weather.h"
	"src/network/Packet100OpenWindow.cpp"
	"src/network/Packet100OpenWindow.h"
	"src/network/Packet101CloseWindow.cpp"
	"src/network/Packet101CloseWindow.h"
	"src/network/Packet102WindowClick.cpp"
	"src/network/Packet102WindowClick.h"
	"src/network/Packet103SetSlot.cpp"
	"src/network/Packet103SetSlot.h"
	"src/network/Packet104WindowItems.cpp"
	"src/network/Packet104WindowItems.h"
	"src/network/Packet105UpdateProgressbar.cpp"
	"src/network/Packet105UpdateProgressbar.h"
	"src/network/Packet106Transaction.cpp"
	"src/network/Packet106Transaction.h"
	"src/network/Packet130UpdateSign.cpp"
	"src/network/Packet130UpdateSign.h"
	"src/network/Packet131MapData.cpp"
	"src/network/Packet131MapData.h"
	"src/network/Packet200Statistic.cpp"
	"src/network/Packet200Statistic.h"
	"src/network/Packet255KickDisconnect.cpp"
	"src/network/Packet255KickDisconnect.h"
	"src/network/PacketRegistration.cpp"
	"src/network/NetHandler.cpp"
	"src/network/NetHandler.h"
	"src/network/NetClientHandler.cpp"
	"src/network/NetClientHandler.h"
	"src/network/NetworkManager.cpp"
	"src/network/NetworkManager.h"
	"src/network/NetworkReaderThread.cpp"
	"src/network/NetworkReaderThread.h"
	"src/network/NetworkWriterThread.cpp"
	"src/network/NetworkWriterThread.h"
	"src/network/NetworkMasterThread.cpp"
	"src/network/NetworkMasterThread.h"
	"src/network/ThreadCloseConnection.cpp"
	"src/network/ThreadCloseConnection.h"
	"src/network/ThreadConnectToServer.cpp"
	"src/network/ThreadConnectToServer.h"
	"src/network/SocketStreams.cpp"
	"src/network/SocketStreams.h"
	
	"src/java/BufferedImage.cpp"
	"src/java/BufferedImage.h"
	"src/java/File.cpp"
	"src/java/File.h"
	"src/java/IOUtil.cpp"
	"src/java/IOUtil.h"
	"src/java/Math.cpp"
	"src/java/Math.h"
	"src/java/Random.cpp"
	"src/java/Random.h"
	"src/java/Resource.h"
	"src/java/Runtime.h"
	"src/java/StrictMath.cpp"
	"src/java/StrictMath.h"
	"src/java/String.cpp"
	"src/java/String.h"
	"src/java/System.cpp"
	"src/java/System.h"
	"src/java/Type.h"
	
	"src/external/stb_image.cpp"
)

# Core object library shared by the client executable and the headless test target.
# An OBJECT library (not STATIC) is used deliberately: every translation unit is
# always linked, so registries relying on translation-unit initialisation cannot
# be silently discarded by the linker.
set(A126_PLATFORM_BACKEND "SDL2" CACHE STRING "Platform backend: SDL2")
set_property(CACHE A126_PLATFORM_BACKEND PROPERTY STRINGS SDL2)
set(A126_PLATFORM_BACKEND_SOURCES)
if(A126_PLATFORM_BACKEND STREQUAL "SDL2")
	list(APPEND A126_PLATFORM_BACKEND_SOURCES
		"src/backends/Platform/SDL2/Platform.cpp"
		"src/backends/Platform/SDL2/Platform.h"
	)
	if(NINTENDO_SWITCH)
		# Upstream SDL2 has no Switch support; use the devkitPro portlib,
		# which is built against Mesa/EGL and the libnx HID/audio services.
		find_package(SDL2 REQUIRED)
		set(A126_PLATFORM_BACKEND_TARGET SDL2::SDL2-static)
	else()
		add_subdirectory("external/SDL2" EXCLUDE_FROM_ALL)
		set(A126_PLATFORM_BACKEND_TARGET SDL2-static)
	endif()
else()
	message(FATAL_ERROR "A126_PLATFORM_BACKEND must be SDL2")
endif()

add_library(Alpha126CppCore OBJECT
	${Alpha126Cpp_SOURCES}
	${A126_PLATFORM_BACKEND_SOURCES}

	"src/pc/CrashHandler.cpp"
	"src/pc/CrashHandler.h"
	"src/pc/OpenGL.h"

	"src/backends/Backend.h"
	"src/backends/Platform/Platform.h"
	"src/legacygl/Context.cpp"
	"src/legacygl/Context.h"
	"src/legacygl/Facade.cpp"
	"src/legacygl/Geometry.h"
	"src/legacygl/LegacyGL.h"
	"src/legacygl/Matrix.cpp"
	"src/legacygl/Matrix.h"
	"src/legacygl/PixelFormat.cpp"
	"src/legacygl/PixelFormat.h"
	"src/legacygl/Primitive.cpp"
	"src/legacygl/Primitive.h"
	"src/legacygl/ResolvedCommands.h"
	"src/legacygl/Sink.h"
	"src/legacygl/Startup.cpp"
	"src/legacygl/Startup.h"
	"src/legacygl/Trace.cpp"
	"src/legacygl/Trace.h"

	"src/pc/external/SDLException.h"
	"src/pc/external/stb_vorbis.cpp"
	"src/pc/external/stb_vorbis.h"

	"src/pc/lwjgl/Gamepad.cpp"
	"src/pc/lwjgl/Gamepad.h"
	"src/pc/lwjgl/Display.cpp"
	"src/pc/lwjgl/Display.h"
	"src/pc/lwjgl/GLContext.cpp"
	"src/pc/lwjgl/GLContext.h"
	"src/pc/lwjgl/Keyboard.cpp"
	"src/pc/lwjgl/Keyboard.h"
	"src/pc/lwjgl/Mouse.cpp"
	"src/pc/lwjgl/Mouse.h"
)

if(NOT NINTENDO_SWITCH)
	# miniaudio is compiled but never called (no ma_* use anywhere in src/), and
	# its POSIX path includes <dlfcn.h>, which newlib does not have.
	target_sources(Alpha126CppCore PRIVATE "src/pc/external/miniaudio.cpp")
endif()

target_include_directories(Alpha126CppCore PUBLIC "src")
target_include_directories(Alpha126CppCore PUBLIC "src/pc")

# OpenAL-Soft headers (for SoundEngine)
target_include_directories(Alpha126CppCore PUBLIC "external/openal-soft/include")

# Compile platform specific code
if(WIN32)
	target_sources(Alpha126CppCore PRIVATE
		"src/pc/win32/java/File.cpp"
		"src/pc/win32/java/Resource.cpp"
		"src/pc/win32/java/Runtime.cpp"
	)

	target_compile_definitions(Alpha126CppCore PUBLIC MC_WIN32)
elseif(APPLE)
	#message(FATAL_ERROR "MacOS is not supported yet.")
	target_sources(Alpha126CppCore PRIVATE
		"src/pc/linux/java/File.cpp"
		"src/pc/linux/java/Resource.cpp"
		"src/pc/linux/java/Runtime.cpp"
	)
elseif(NINTENDO_SWITCH)
	# The POSIX File/Resource work against libnx's devoptab (romfs:/sdmc:), but
	# Runtime needs the malloc arena rather than the stubbed Linux values.
	target_sources(Alpha126CppCore PRIVATE
		"src/pc/linux/java/File.cpp"
		"src/pc/linux/java/Resource.cpp"
		"src/pc/switch/java/Runtime.cpp"
		"src/pc/switch/SwitchRuntime.cpp"
		"src/pc/switch/SwitchRuntime.h"
	)
elseif(UNIX)
	target_sources(Alpha126CppCore PRIVATE
		"src/pc/linux/java/File.cpp"
		"src/pc/linux/java/Resource.cpp"
		"src/pc/linux/java/Runtime.cpp"
	)

	target_compile_definitions(Alpha126CppCore PUBLIC MC_LINUX)
endif()

# Use C++17 (required for std::variant in DataWatcher)
set_target_properties(Alpha126CppCore PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

# Performance optimizations for Release builds
if(MSVC)
	# MSVC: /O2 = maximize speed, /Ob2 = aggressive inlining, /Oi = generate intrinsic functions
	# /GL = whole program optimization (works with LTCG)
	target_compile_options(Alpha126CppCore PUBLIC
		$<$<CONFIG:Release>:/O2 /Ob2 /Oi /GL>
		$<$<CONFIG:RelWithDebInfo>:/O2 /Ob2 /Oi /GL>
	)
	# Link-time optimizations (required when using /GL to avoid warning)
	target_link_options(Alpha126CppCore PUBLIC
		$<$<CONFIG:Release>:/LTCG>
		$<$<CONFIG:RelWithDebInfo>:/LTCG>
	)
else()
	# GCC/Clang: -O3 = maximum optimization, -march=native = use CPU-specific optimizations
	# -funroll-loops = unroll loops for better performance
	# -finline-functions = aggressive function inlining
	# -fomit-frame-pointer = omit frame pointer (frees a register, only for Release)
	if(CMAKE_CROSSCOMPILING)
		# Cross builds must not override the toolchain's -march (devkitPro
		# pins -march=armv8-a+crc+crypto for the Switch), and LTO across
		# switch.specs/libnx is not supported. Frame pointers are kept so
		# crash reports stay unwindable.
		set(A126_RELEASE_OPTS -O3 -funroll-loops -finline-functions)
		set(A126_RELEASE_LINK_OPTS)
	else()
		set(A126_RELEASE_OPTS -O3 -march=native -funroll-loops -finline-functions)
		set(A126_RELEASE_LINK_OPTS -flto)
	endif()
	target_compile_options(Alpha126CppCore PUBLIC
		# -ffp-contract=off keeps Java bit-parity: GCC's default
		# -ffp-contract=fast fuses multiply-adds inside StrictMath's fdlibm
		# log transcription and shifts nextGaussian by one ULP. AArch64 has
		# FMADD unconditionally, so this matters even without -march=native.
		-ffp-contract=off
		$<$<CONFIG:Release>:${A126_RELEASE_OPTS} $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:-fomit-frame-pointer>>
		$<$<CONFIG:RelWithDebInfo>:${A126_RELEASE_OPTS}>
	)
	# Link-time optimizations
	target_link_options(Alpha126CppCore PUBLIC
		$<$<CONFIG:Release>:${A126_RELEASE_LINK_OPTS}>
		$<$<CONFIG:RelWithDebInfo>:${A126_RELEASE_LINK_OPTS}>
	)
endif()

# Third party
target_link_libraries(Alpha126CppCore PUBLIC ${A126_PLATFORM_BACKEND_TARGET})

add_subdirectory("external/glad" EXCLUDE_FROM_ALL)
if(NINTENDO_SWITCH)
	# newlib has no <dlfcn.h> and there is no libGL to dlopen; the game only
	# ever loads GL through gladLoadGLLoader(SDL_GL_GetProcAddress), so the
	# built-in dynamic loader is compiled out.
	target_compile_definitions(glad PRIVATE GLAD_NO_LOADER)
endif()

if(DEFINED A126_RENDER_BACKEND)
	message(STATUS "Ignoring deprecated A126_RENDER_BACKEND=${A126_RENDER_BACKEND}; select the renderer at runtime")
	unset(A126_RENDER_BACKEND CACHE)
	unset(A126_RENDER_BACKEND)
endif()

if(NINTENDO_SWITCH)
	# No Vulkan driver exists in the homebrew toolchain (deko3d is the native
	# low-level API), and the block below requires a host Vulkan SDK + glslc.
	set(A126_ENABLE_VULKAN_BACKEND OFF)
else()
	option(A126_ENABLE_VULKAN_BACKEND "Build the Vulkan renderer backend" ON)
endif()
if(WIN32)
	option(A126_ENABLE_D3D12_BACKEND "Build the Direct3D 12 renderer backend" ON)
else()
	set(A126_ENABLE_D3D12_BACKEND OFF)
endif()
option(A126_ENABLE_GL_GPU_TESTS "Enable serialized LegacyGL GPU parity tests" OFF)
option(A126_ENABLE_RENDER_DIAGNOSTICS "Compile renderer diagnostics instrumentation" OFF)
# Profiling probe build: appends memory and per-subsystem timings to
# a126cpp-profile.txt beside the executable. Off by default; the shipping build
# carries no instrumentation.
option(A126_ENABLE_MEMORY_PROBE "Compile the memory and timing probe" OFF)
if(A126_ENABLE_MEMORY_PROBE)
	target_compile_definitions(Alpha126CppCore PUBLIC A126_ENABLE_MEMORY_PROBE=1)
endif()
if(NINTENDO_SWITCH)
	set(A126_INITIAL_RENDER_BACKEND "nativegl")
else()
	set(A126_INITIAL_RENDER_BACKEND "gl21")
endif()
set(A126_DEFAULT_RENDER_BACKEND "${A126_INITIAL_RENDER_BACKEND}" CACHE STRING
	"Default renderer: nativegl, gl21, gl33, vulkan or d3d12")
unset(A126_INITIAL_RENDER_BACKEND)
set_property(CACHE A126_DEFAULT_RENDER_BACKEND PROPERTY STRINGS nativegl gl21 gl33 vulkan d3d12)
set(A126_EXECUTABLE_NAME "Alpha126Cpp" CACHE STRING "Client executable output name")

if(A126_DEFAULT_RENDER_BACKEND STREQUAL "native")
	message(STATUS "Migrating A126_DEFAULT_RENDER_BACKEND=native to nativegl")
	set(A126_DEFAULT_RENDER_BACKEND "nativegl" CACHE STRING "Default renderer" FORCE)
endif()
if(NOT A126_DEFAULT_RENDER_BACKEND MATCHES "^(nativegl|gl21|gl33|vulkan|d3d12)$")
	message(FATAL_ERROR "Unknown A126_DEFAULT_RENDER_BACKEND=${A126_DEFAULT_RENDER_BACKEND}")
endif()
if(A126_DEFAULT_RENDER_BACKEND STREQUAL "vulkan" AND NOT A126_ENABLE_VULKAN_BACKEND)
	message(FATAL_ERROR "The default Vulkan renderer is disabled")
endif()
if(A126_DEFAULT_RENDER_BACKEND STREQUAL "d3d12" AND NOT A126_ENABLE_D3D12_BACKEND)
	message(FATAL_ERROR "The default D3D12 renderer is disabled")
endif()

add_library(a126cpp-render-backend-opengl-context STATIC EXCLUDE_FROM_ALL
	"src/backends/OpenGL/Context.cpp"
	"src/backends/OpenGL/Context.h"
)
target_include_directories(a126cpp-render-backend-opengl-context PRIVATE "src")
target_link_libraries(a126cpp-render-backend-opengl-context PUBLIC glad ${A126_PLATFORM_BACKEND_TARGET})
set_target_properties(a126cpp-render-backend-opengl-context PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

add_library(a126cpp-render-backend-nativegl STATIC EXCLUDE_FROM_ALL
	"src/backends/NativeGL/Backend.cpp"
	"src/backends/OpenGL21/Backend.cpp"
)
target_include_directories(a126cpp-render-backend-nativegl PRIVATE "src")
target_link_libraries(a126cpp-render-backend-nativegl PRIVATE a126cpp-render-backend-opengl-context)
set_target_properties(a126cpp-render-backend-nativegl PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

add_library(a126cpp-render-backend-opengl33 STATIC EXCLUDE_FROM_ALL
	"src/backends/OpenGL33/Backend.cpp"
	"src/backends/OpenGL33/Shaders.cpp"
	"src/backends/OpenGL33/Shaders.h"
)
target_include_directories(a126cpp-render-backend-opengl33 PRIVATE "src")
target_link_libraries(a126cpp-render-backend-opengl33 PRIVATE a126cpp-render-backend-opengl-context)
set_target_properties(a126cpp-render-backend-opengl33 PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

if(A126_ENABLE_VULKAN_BACKEND)
	set(A126_VULKAN_SDK_ROOT "" CACHE PATH "Vulkan SDK root override")
	set(A126_VULKAN_SDK_CANDIDATES)
	if(A126_VULKAN_SDK_ROOT)
		list(APPEND A126_VULKAN_SDK_CANDIDATES "${A126_VULKAN_SDK_ROOT}")
	endif()
	if(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
		file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" A126_VULKAN_SDK_FROM_ENV)
		list(APPEND A126_VULKAN_SDK_CANDIDATES "${A126_VULKAN_SDK_FROM_ENV}")
	endif()
	if(WIN32)
		file(GLOB A126_VULKAN_STANDARD_ROOTS LIST_DIRECTORIES true "C:/VulkanSDK/*")
		list(SORT A126_VULKAN_STANDARD_ROOTS ORDER DESCENDING)
		list(APPEND A126_VULKAN_SDK_CANDIDATES ${A126_VULKAN_STANDARD_ROOTS})
	endif()
	list(REMOVE_DUPLICATES A126_VULKAN_SDK_CANDIDATES)

	if(A126_VULKAN_SDK_ROOT)
		set(Vulkan_ROOT "${A126_VULKAN_SDK_ROOT}")
	endif()
	find_package(Vulkan QUIET)
	if(NOT Vulkan_FOUND AND WIN32)
		# LunarG's installer does not update an already-running process's
		# environment. Fall back to the versioned install roots so a fresh
		# configure works from that process without hand-editing VULKAN_SDK.
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(A126_VULKAN_LIBRARY_DIR "Lib")
		else()
			set(A126_VULKAN_LIBRARY_DIR "Lib32")
		endif()
		foreach(A126_VULKAN_SDK_CANDIDATE IN LISTS A126_VULKAN_SDK_CANDIDATES)
			if(EXISTS "${A126_VULKAN_SDK_CANDIDATE}/Include/vulkan/vulkan.h" AND
				EXISTS "${A126_VULKAN_SDK_CANDIDATE}/${A126_VULKAN_LIBRARY_DIR}/vulkan-1.lib")
				add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
				set_target_properties(Vulkan::Vulkan PROPERTIES
					IMPORTED_LOCATION "${A126_VULKAN_SDK_CANDIDATE}/${A126_VULKAN_LIBRARY_DIR}/vulkan-1.lib"
					INTERFACE_INCLUDE_DIRECTORIES "${A126_VULKAN_SDK_CANDIDATE}/Include"
				)
				set(Vulkan_FOUND TRUE)
				message(STATUS "Found Vulkan SDK fallback: ${A126_VULKAN_SDK_CANDIDATE}")
				break()
			endif()
		endforeach()
	endif()
	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR "A126_ENABLE_VULKAN_BACKEND=ON requires the Vulkan SDK")
	endif()

	set(A126_VULKAN_TOOL_HINTS)
	foreach(A126_VULKAN_SDK_CANDIDATE IN LISTS A126_VULKAN_SDK_CANDIDATES)
		list(APPEND A126_VULKAN_TOOL_HINTS
			"${A126_VULKAN_SDK_CANDIDATE}/Bin"
			"${A126_VULKAN_SDK_CANDIDATE}/bin"
		)
	endforeach()
	find_program(A126_VULKAN_GLSLC_EXECUTABLE
		NAMES glslc glslc.exe
		HINTS ${A126_VULKAN_TOOL_HINTS}
	)
	if(NOT A126_VULKAN_GLSLC_EXECUTABLE)
		message(FATAL_ERROR "The Vulkan backend requires glslc from the Vulkan SDK")
	endif()
	find_program(A126_VULKAN_SPIRV_VAL_EXECUTABLE
		NAMES spirv-val spirv-val.exe
		HINTS ${A126_VULKAN_TOOL_HINTS}
	)

	set(A126_VULKAN_GENERATED_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/vulkan")
	set(A126_VULKAN_SHADER_OUTPUTS)
	function(a126_add_vulkan_shader source name)
		set(shader_spv "${A126_VULKAN_GENERATED_SHADER_DIR}/${name}.spv")
		set(shader_inc "${A126_VULKAN_GENERATED_SHADER_DIR}/${name}.inc")
		if(A126_VULKAN_SPIRV_VAL_EXECUTABLE)
			add_custom_command(
				OUTPUT "${shader_spv}" "${shader_inc}"
				COMMAND ${CMAKE_COMMAND} -E make_directory "${A126_VULKAN_GENERATED_SHADER_DIR}"
				COMMAND "${A126_VULKAN_GLSLC_EXECUTABLE}" --target-env=vulkan1.1 "${source}" -o "${shader_spv}"
				COMMAND "${A126_VULKAN_SPIRV_VAL_EXECUTABLE}" --target-env vulkan1.1 "${shader_spv}"
				COMMAND "${A126_VULKAN_GLSLC_EXECUTABLE}" --target-env=vulkan1.1 -mfmt=c "${source}" -o "${shader_inc}"
				DEPENDS "${source}"
				COMMENT "Compiling and validating Vulkan shader ${name}"
				VERBATIM
			)
		else()
			add_custom_command(
				OUTPUT "${shader_spv}" "${shader_inc}"
				COMMAND ${CMAKE_COMMAND} -E make_directory "${A126_VULKAN_GENERATED_SHADER_DIR}"
				COMMAND "${A126_VULKAN_GLSLC_EXECUTABLE}" --target-env=vulkan1.1 "${source}" -o "${shader_spv}"
				COMMAND "${A126_VULKAN_GLSLC_EXECUTABLE}" --target-env=vulkan1.1 -mfmt=c "${source}" -o "${shader_inc}"
				DEPENDS "${source}"
				COMMENT "Compiling Vulkan shader ${name}"
				VERBATIM
			)
		endif()
		set(A126_VULKAN_SHADER_OUTPUTS ${A126_VULKAN_SHADER_OUTPUTS} "${shader_spv}" "${shader_inc}" PARENT_SCOPE)
	endfunction()

	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/LegacyFFP.vert" "LegacyFFP.vert")
	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/LegacyFFP.frag" "LegacyFFP.frag")
	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/Clear.vert" "Clear.vert")
	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/Clear.frag" "Clear.frag")
	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/Present.vert" "Present.vert")
	a126_add_vulkan_shader("${CMAKE_CURRENT_SOURCE_DIR}/src/backends/Vulkan/shaders/Present.frag" "Present.frag")
	set_source_files_properties(${A126_VULKAN_SHADER_OUTPUTS} PROPERTIES GENERATED TRUE HEADER_FILE_ONLY TRUE)
	add_custom_target(a126cpp-vulkan-shaders DEPENDS ${A126_VULKAN_SHADER_OUTPUTS})

	add_library(a126cpp-render-backend-vulkan STATIC EXCLUDE_FROM_ALL
		"src/backends/Vulkan/Backend.cpp"
		"src/backends/Vulkan/PipelineCacheFile.cpp"
		"src/backends/Vulkan/PipelineCacheFile.h"
		"src/backends/Vulkan/Shaders.cpp"
		"src/backends/Vulkan/Shaders.h"
		${A126_VULKAN_SHADER_OUTPUTS}
	)
	add_dependencies(a126cpp-render-backend-vulkan a126cpp-vulkan-shaders)
	target_include_directories(a126cpp-render-backend-vulkan PRIVATE
		"src"
		"${A126_VULKAN_GENERATED_SHADER_DIR}"
		"$<TARGET_PROPERTY:Vulkan::Vulkan,INTERFACE_INCLUDE_DIRECTORIES>"
	)
	if(A126_ENABLE_RENDER_DIAGNOSTICS)
		target_compile_definitions(a126cpp-render-backend-vulkan PRIVATE
			A126_RENDER_DIAGNOSTICS_COMPILED=1)
	endif()
	set_target_properties(a126cpp-render-backend-vulkan PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
	)
endif()

if(WIN32 AND A126_ENABLE_D3D12_BACKEND)
	add_library(a126cpp-render-backend-d3d12 STATIC EXCLUDE_FROM_ALL
		"src/backends/D3D12/Backend.cpp"
		"src/backends/D3D12/Shaders.cpp"
		"src/backends/D3D12/Shaders.h"
	)
	target_include_directories(a126cpp-render-backend-d3d12 PRIVATE "src")
	target_link_libraries(a126cpp-render-backend-d3d12 PRIVATE d3d12 dxgi d3dcompiler)
	if(A126_ENABLE_RENDER_DIAGNOSTICS)
		target_compile_definitions(a126cpp-render-backend-d3d12 PRIVATE
			A126_RENDER_DIAGNOSTICS_COMPILED=1)
	endif()
	set_target_properties(a126cpp-render-backend-d3d12 PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
	)
endif()

# zlib ships add_test(example ...) hooks whose executables we do not build;
# disable the example targets so ctest does not report them as missing tests.
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Build zlib examples" FORCE)
add_subdirectory("external/zlib" EXCLUDE_FROM_ALL)

target_link_libraries(Alpha126CppCore PUBLIC zlibstatic)
target_include_directories(Alpha126CppCore PUBLIC "external/zlib")
target_include_directories(Alpha126CppCore PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/external/zlib")

target_include_directories(Alpha126CppCore PUBLIC "external/stb")
target_include_directories(Alpha126CppCore PUBLIC "external/gzip-hpp/include")
if(NINTENDO_SWITCH)
	# newlib gates POSIX/BSD declarations behind __POSIX_VISIBLE/__BSD_VISIBLE,
	# which -std=c++17 (CXX_EXTENSIONS NO) switches off. glibc exposes them by
	# default, which is why the Linux build needs no equivalent. Required for
	# timeval, IP_TOS and gethostbyname in src/network.
	target_compile_definitions(Alpha126CppCore PUBLIC _GNU_SOURCE)
endif()

target_include_directories(Alpha126CppCore PUBLIC "external/miniaudio")

if(NINTENDO_SWITCH)
	# Static-only platform: force a static archive (also required so consumers
	# see undecorated AL_API), keep only the null + libnx audout backends, and
	# pre-seed the probes that would otherwise test x86/hosted-OS assumptions.
	set(LIBTYPE STATIC CACHE STRING "OpenAL-Soft library type" FORCE)
	set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
	set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(ALSOFT_CONFIG OFF CACHE BOOL "" FORCE)
	set(ALSOFT_DLOPEN OFF CACHE BOOL "" FORCE)
	set(ALSOFT_CPUEXT_SSE OFF CACHE BOOL "" FORCE)
	set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "" FORCE)
	set(HAVE_MSSE_SWITCH 0 CACHE INTERNAL "")
	set(HAVE_PTHREAD 0 CACHE INTERNAL "")
	set(HAVE_GCC_PROTECTED_VISIBILITY 0 CACHE INTERNAL "")
	set(HAVE_GCC_DEFAULT_VISIBILITY 0 CACHE INTERNAL "")
	set(HAVE_GETTIMEOFDAY 1 CACHE INTERNAL "")
	set(HAVE_NANOSLEEP 1 CACHE INTERNAL "")
endif()
# OpenAL-Soft library - build from source
# On Windows, the target is "OpenAL32", on other platforms it's "openal"
add_subdirectory("external/openal-soft" EXCLUDE_FROM_ALL)

# Link against the built OpenAL library
# OpenAL-Soft's CMakeLists.txt creates a target named ${LIBNAME}
# which is "OpenAL32" on Windows and "openal" on other platforms
if(WIN32)
	if(TARGET OpenAL32)
		target_link_libraries(Alpha126CppCore PUBLIC OpenAL32)
		# The bundled OpenAL-Soft null/wave backends call timeGetTime directly.
		# Its old CMake target omits the Windows import library with current
		# toolchains, so make the platform dependency explicit.
		target_link_libraries(OpenAL32 winmm)
	else()
		message(WARNING "OpenAL32 target not found. Please check OpenAL-Soft CMakeLists.txt")
	endif()
else()
	if(TARGET openal)
		target_link_libraries(Alpha126CppCore PUBLIC openal)
	else()
		message(WARNING "openal target not found. Please check OpenAL-Soft CMakeLists.txt")
	endif()
endif()
if(NINTENDO_SWITCH)
	# The game's TUs include AL headers through this directory scope, so they
	# must see the same static-linkage declarations as the library build.
	target_compile_definitions(Alpha126CppCore PUBLIC AL_LIBTYPE_STATIC)
endif()

# Client executable
add_executable(Alpha126Cpp
	"src/backends/Backend.cpp"
	"src/pc/Main.cpp"
	"src/tools/BenchUtil.cpp"
	"src/tools/BenchUtil.h"
	"src/tools/SceneCapture.cpp"
	"src/tools/SceneCapture.h"
	"src/tools/SignBench.cpp"
	"src/tools/SignBench.h"
)
target_compile_definitions(Alpha126Cpp PRIVATE
	A126_HAS_BACKEND_NATIVEGL=1
	A126_HAS_BACKEND_OPENGL21=1
	A126_HAS_BACKEND_OPENGL33=1
	A126_DEFAULT_RENDER_BACKEND="${A126_DEFAULT_RENDER_BACKEND}"
)
target_link_libraries(Alpha126Cpp PRIVATE
	Alpha126CppCore
	a126cpp-render-backend-nativegl
	a126cpp-render-backend-opengl33
)
if(A126_ENABLE_VULKAN_BACKEND)
	target_compile_definitions(Alpha126Cpp PRIVATE A126_HAS_BACKEND_VULKAN=1)
	target_link_libraries(Alpha126Cpp PRIVATE a126cpp-render-backend-vulkan)
endif()
if(A126_ENABLE_D3D12_BACKEND)
	target_compile_definitions(Alpha126Cpp PRIVATE A126_HAS_BACKEND_D3D12=1)
	target_link_libraries(Alpha126Cpp PRIVATE a126cpp-render-backend-d3d12)
endif()

if(WIN32)
	target_sources(Alpha126Cpp PRIVATE
		"src/pc/win32/Alpha126Cpp.rc"
	)

	# Set icon file path for resource compiler
	set_source_files_properties("src/pc/win32/Alpha126Cpp.rc" PROPERTIES
		VS_RESOURCE_GENERATOR "rc"
	)
endif()

set_target_properties(Alpha126Cpp PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
	OUTPUT_NAME "${A126_EXECUTABLE_NAME}"
)

if(NOT CMAKE_CROSSCOMPILING)
# Headless deterministic regression suite.
# Links the same production code as the client but never initialises SDL,
# OpenGL, OpenAL or any input device.
#
# Test files are globbed on purpose: a new regression test is a single new file
# under src/tools/headless/tests and needs no build edit. CONFIGURE_DEPENDS
# makes the build re-glob when that directory changes.
file(GLOB Alpha126Cpp_HEADLESS_TESTS CONFIGURE_DEPENDS
	"src/tools/headless/tests/*.cpp"
)

add_executable(a126cpp-headless-tests
	"src/backends/Backend.cpp"
	"src/backends/Vulkan/PipelineCacheFile.cpp"
	"src/tools/headless/TestFramework.cpp"
	"src/tools/headless/TestFramework.h"
	"src/tools/headless/TestWorld.cpp"
	"src/tools/headless/TestWorld.h"
	"src/tools/headless/HeadlessMain.cpp"

	${Alpha126Cpp_HEADLESS_TESTS}
)
target_compile_definitions(a126cpp-headless-tests PRIVATE A126_HAS_BACKEND_NATIVEGL=1)
target_link_libraries(a126cpp-headless-tests PRIVATE Alpha126CppCore a126cpp-render-backend-nativegl)

set_target_properties(a126cpp-headless-tests PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

# Stress and profiling harness. Links the same production code and backends as
# the client, drives it through scripted worst-case workloads in a hidden
# window, and logs per-phase timings and memory. Built with
# A126_ENABLE_MEMORY_PROBE=ON it also writes the probe's bucket reports.
add_executable(a126cpp-stress
	"src/backends/Backend.cpp"
	"src/tools/BenchUtil.cpp"
	"src/tools/BenchUtil.h"
	"src/tools/stress/Scenarios.cpp"
	"src/tools/stress/StressHarness.cpp"
	"src/tools/stress/StressHarness.h"
	"src/tools/stress/StressMain.cpp"
)
target_compile_definitions(a126cpp-stress PRIVATE
	A126_HAS_BACKEND_NATIVEGL=1
	A126_HAS_BACKEND_OPENGL21=1
	A126_HAS_BACKEND_OPENGL33=1
	A126_DEFAULT_RENDER_BACKEND="${A126_DEFAULT_RENDER_BACKEND}"
)
target_link_libraries(a126cpp-stress PRIVATE
	Alpha126CppCore
	a126cpp-render-backend-nativegl
	a126cpp-render-backend-opengl33
)
if(A126_ENABLE_VULKAN_BACKEND)
	target_compile_definitions(a126cpp-stress PRIVATE A126_HAS_BACKEND_VULKAN=1)
	target_link_libraries(a126cpp-stress PRIVATE a126cpp-render-backend-vulkan)
endif()
if(A126_ENABLE_D3D12_BACKEND)
	target_compile_definitions(a126cpp-stress PRIVATE A126_HAS_BACKEND_D3D12=1)
	target_link_libraries(a126cpp-stress PRIVATE a126cpp-render-backend-d3d12)
endif()
set_target_properties(a126cpp-stress PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

# GL inventory checker. Standalone on purpose: it only reads source text, so it
# does not link the game and stays fast enough to run on every build.
add_executable(a126cpp-gl-inventory
	"src/tools/GlInventory.cpp"
)
target_compile_definitions(a126cpp-gl-inventory PRIVATE
	A126_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/src"
)
set_target_properties(a126cpp-gl-inventory PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)
endif()

if(NINTENDO_SWITCH)
# Stress harness on the console: same sources as the desktop harness, Switch
# backends, probe-driven. hbmenu passes no argv, so the binary reads
# sdmc:/switch/a126cpp-args.txt (see StressMain.cpp). Assets ride in romfs
# exactly like the client.
add_executable(a126cpp-stress-switch
	"src/backends/Backend.cpp"
	"src/tools/BenchUtil.cpp"
	"src/tools/BenchUtil.h"
	"src/tools/stress/Scenarios.cpp"
	"src/tools/stress/StressHarness.cpp"
	"src/tools/stress/StressHarness.h"
	"src/tools/stress/StressMain.cpp"
)
target_compile_definitions(a126cpp-stress-switch PRIVATE
	A126_HAS_BACKEND_NATIVEGL=1
	A126_HAS_BACKEND_OPENGL21=1
	A126_HAS_BACKEND_OPENGL33=1
	A126_DEFAULT_RENDER_BACKEND="${A126_DEFAULT_RENDER_BACKEND}"
)
target_link_libraries(a126cpp-stress-switch PRIVATE
	Alpha126CppCore
	a126cpp-render-backend-nativegl
	a126cpp-render-backend-opengl33
)
set_target_properties(a126cpp-stress-switch PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)
nx_generate_nacp(a126cpp-stress-switch.nacp
	NAME "a126cpp stress"
	AUTHOR "a126cpp"
	VERSION "${PROJECT_VERSION}"
)
nx_create_nro(a126cpp-stress-switch
	NACP "${CMAKE_CURRENT_BINARY_DIR}/a126cpp-stress-switch.nacp"
	ROMFS "${CMAKE_SOURCE_DIR}/resource"
)
endif()

if(A126_ENABLE_GL_GPU_TESTS)
	# Each fixture process links exactly one backend. Comparison does not create
	# a context, so either executable can compare the two text records.
	add_executable(a126cpp-legacygl-gpu-parity-native
		"src/backends/Backend.cpp"
		"src/tools/LegacyGLGpuParity.cpp"
	)
	target_compile_definitions(a126cpp-legacygl-gpu-parity-native PRIVATE A126_HAS_BACKEND_NATIVEGL=1)
	target_link_libraries(a126cpp-legacygl-gpu-parity-native PRIVATE
		Alpha126CppCore a126cpp-render-backend-nativegl
	)

	add_executable(a126cpp-legacygl-gpu-parity-gl21
		"src/backends/Backend.cpp"
		"src/tools/LegacyGLGpuParity.cpp"
	)
	target_compile_definitions(a126cpp-legacygl-gpu-parity-gl21 PRIVATE A126_HAS_BACKEND_OPENGL21=1)
	target_link_libraries(a126cpp-legacygl-gpu-parity-gl21 PRIVATE
		Alpha126CppCore a126cpp-render-backend-nativegl
	)

	add_executable(a126cpp-legacygl-gpu-parity-gl33
		"src/backends/Backend.cpp"
		"src/tools/LegacyGLGpuParity.cpp"
		"src/backends/OpenGL33/tests/SyncTests.cpp"
	)
	target_compile_definitions(a126cpp-legacygl-gpu-parity-gl33 PRIVATE A126_HAS_BACKEND_OPENGL33=1)
	target_link_libraries(a126cpp-legacygl-gpu-parity-gl33 PRIVATE
		Alpha126CppCore a126cpp-render-backend-opengl33 glad
	)

	set(A126_GL_GPU_PARITY_TARGETS
		a126cpp-legacygl-gpu-parity-native
		a126cpp-legacygl-gpu-parity-gl21
		a126cpp-legacygl-gpu-parity-gl33
	)
	if(A126_ENABLE_VULKAN_BACKEND)
		add_executable(a126cpp-legacygl-gpu-parity-vulkan
			"src/backends/Backend.cpp"
			"src/tools/LegacyGLGpuParity.cpp"
		)
		target_compile_definitions(a126cpp-legacygl-gpu-parity-vulkan PRIVATE A126_HAS_BACKEND_VULKAN=1)
		target_link_libraries(a126cpp-legacygl-gpu-parity-vulkan PRIVATE
			Alpha126CppCore a126cpp-render-backend-vulkan
		)
		list(APPEND A126_GL_GPU_PARITY_TARGETS a126cpp-legacygl-gpu-parity-vulkan)
	endif()
	if(A126_ENABLE_D3D12_BACKEND)
		add_executable(a126cpp-legacygl-gpu-parity-d3d12
			"src/backends/Backend.cpp"
			"src/tools/LegacyGLGpuParity.cpp"
		)
		target_compile_definitions(a126cpp-legacygl-gpu-parity-d3d12 PRIVATE A126_HAS_BACKEND_D3D12=1)
		target_link_libraries(a126cpp-legacygl-gpu-parity-d3d12 PRIVATE
			Alpha126CppCore a126cpp-render-backend-d3d12
		)
		list(APPEND A126_GL_GPU_PARITY_TARGETS a126cpp-legacygl-gpu-parity-d3d12)
	endif()

	set_target_properties(${A126_GL_GPU_PARITY_TARGETS} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
	)
endif()

# Output to bin/ directory
set_target_properties(Alpha126Cpp PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
if(NOT CMAKE_CROSSCOMPILING)
	set_target_properties(a126cpp-headless-tests a126cpp-gl-inventory a126cpp-stress PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
	)
endif()

if(WIN32 AND TARGET OpenAL32)
	# Copy DLL to output directory for runtime (Windows only)
	add_custom_command(TARGET Alpha126Cpp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:OpenAL32>"
			"$<TARGET_FILE_DIR:Alpha126Cpp>/OpenAL32.dll"
		COMMENT "Copying OpenAL32.dll to output directory"
	)
	add_custom_command(TARGET a126cpp-headless-tests POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:OpenAL32>"
			"$<TARGET_FILE_DIR:a126cpp-headless-tests>/OpenAL32.dll"
		COMMENT "Copying OpenAL32.dll to output directory"
	)
	add_custom_command(TARGET a126cpp-stress POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:OpenAL32>"
			"$<TARGET_FILE_DIR:a126cpp-stress>/OpenAL32.dll"
		COMMENT "Copying OpenAL32.dll to output directory"
	)
	if(A126_ENABLE_GL_GPU_TESTS)
		add_custom_command(TARGET a126cpp-legacygl-gpu-parity-native POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"$<TARGET_FILE:OpenAL32>"
				"$<TARGET_FILE_DIR:a126cpp-legacygl-gpu-parity-native>/OpenAL32.dll"
			COMMENT "Copying OpenAL32.dll to output directory"
		)
		add_custom_command(TARGET a126cpp-legacygl-gpu-parity-gl21 POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"$<TARGET_FILE:OpenAL32>"
				"$<TARGET_FILE_DIR:a126cpp-legacygl-gpu-parity-gl21>/OpenAL32.dll"
			COMMENT "Copying OpenAL32.dll to output directory"
		)
		add_custom_command(TARGET a126cpp-legacygl-gpu-parity-gl33 POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"$<TARGET_FILE:OpenAL32>"
				"$<TARGET_FILE_DIR:a126cpp-legacygl-gpu-parity-gl33>/OpenAL32.dll"
			COMMENT "Copying OpenAL32.dll to output directory"
		)
		if(A126_ENABLE_VULKAN_BACKEND)
			add_custom_command(TARGET a126cpp-legacygl-gpu-parity-vulkan POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					"$<TARGET_FILE:OpenAL32>"
					"$<TARGET_FILE_DIR:a126cpp-legacygl-gpu-parity-vulkan>/OpenAL32.dll"
				COMMENT "Copying OpenAL32.dll to output directory"
			)
		endif()
		if(A126_ENABLE_D3D12_BACKEND)
			add_custom_command(TARGET a126cpp-legacygl-gpu-parity-d3d12 POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					"$<TARGET_FILE:OpenAL32>"
					"$<TARGET_FILE_DIR:a126cpp-legacygl-gpu-parity-d3d12>/OpenAL32.dll"
				COMMENT "Copying OpenAL32.dll to output directory"
			)
		endif()
	endif()
endif()

if(NINTENDO_SWITCH)
	# Assets ship inside the NRO's romfs (File::openResourceDirectory returns
	# romfs:/ on Switch), so no loose resource copy is needed.
	nx_generate_nacp(Alpha126Cpp.nacp
		NAME "Minecraft Alpha v1.2.6"
		AUTHOR "a126cpp"
		VERSION "${PROJECT_VERSION}"
	)
	nx_create_nro(Alpha126Cpp
		NACP "${CMAKE_CURRENT_BINARY_DIR}/Alpha126Cpp.nacp"
		ROMFS "${CMAKE_SOURCE_DIR}/resource"
	)
else()
	# Copy resource folder to output directory next to executable
	# Use copy_directory_if_different to avoid errors if directory doesn't exist
	add_custom_command(TARGET Alpha126Cpp POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Alpha126Cpp>"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_SOURCE_DIR}/resource"
			"$<TARGET_FILE_DIR:Alpha126Cpp>/resource"
		COMMENT "Copying resource folder to output directory"
	)
endif()

if(NOT CMAKE_CROSSCOMPILING)
enable_testing()
add_test(NAME headless COMMAND a126cpp-headless-tests --all)

# The frontend surface must keep matching what the renderer issues. A new GL
# call without a semantic implementation fails this test.
add_test(NAME gl-inventory COMMAND a126cpp-gl-inventory)

if(A126_ENABLE_GL_GPU_TESTS)
	set(A126_GL_GPU_NATIVE_RECORD "${CMAKE_CURRENT_BINARY_DIR}/legacygl-gpu-native.txt")
	set(A126_GL_GPU_GL21_RECORD "${CMAKE_CURRENT_BINARY_DIR}/legacygl-gpu-gl21.txt")
	set(A126_GL_GPU_GL33_RECORD "${CMAKE_CURRENT_BINARY_DIR}/legacygl-gpu-gl33.txt")
	set(A126_GL_GPU_TESTS
		legacygl-gpu-record-native
		legacygl-gpu-record-gl21
		legacygl-gpu-record-gl33
		legacygl-gpu-compare-gl21
		legacygl-gpu-compare
		legacygl-gpu-compare-gl21-gl33
	)

	add_test(NAME legacygl-gpu-record-native
		COMMAND a126cpp-legacygl-gpu-parity-native --record "${A126_GL_GPU_NATIVE_RECORD}"
	)
	set_tests_properties(legacygl-gpu-record-native PROPERTIES
		FIXTURES_SETUP legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	add_test(NAME legacygl-gpu-record-gl21
		COMMAND a126cpp-legacygl-gpu-parity-gl21 --record "${A126_GL_GPU_GL21_RECORD}"
	)
	set_tests_properties(legacygl-gpu-record-gl21 PROPERTIES
		FIXTURES_SETUP legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	add_test(NAME legacygl-gpu-record-gl33
		COMMAND a126cpp-legacygl-gpu-parity-gl33 --record "${A126_GL_GPU_GL33_RECORD}"
	)
	set_tests_properties(legacygl-gpu-record-gl33 PROPERTIES
		FIXTURES_SETUP legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	add_test(NAME legacygl-gpu-compare-gl21
		COMMAND a126cpp-legacygl-gpu-parity-native --compare "${A126_GL_GPU_NATIVE_RECORD}" "${A126_GL_GPU_GL21_RECORD}"
	)
	set_tests_properties(legacygl-gpu-compare-gl21 PROPERTIES
		FIXTURES_REQUIRED legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	add_test(NAME legacygl-gpu-compare
		COMMAND a126cpp-legacygl-gpu-parity-native --compare "${A126_GL_GPU_NATIVE_RECORD}" "${A126_GL_GPU_GL33_RECORD}"
	)
	set_tests_properties(legacygl-gpu-compare PROPERTIES
		FIXTURES_REQUIRED legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	add_test(NAME legacygl-gpu-compare-gl21-gl33
		COMMAND a126cpp-legacygl-gpu-parity-gl21 --compare "${A126_GL_GPU_GL21_RECORD}" "${A126_GL_GPU_GL33_RECORD}"
	)
	set_tests_properties(legacygl-gpu-compare-gl21-gl33 PROPERTIES
		FIXTURES_REQUIRED legacygl-gpu-records
		RESOURCE_LOCK legacygl-gpu
	)

	if(A126_ENABLE_VULKAN_BACKEND)
		set(A126_GL_GPU_VULKAN_RECORD "${CMAKE_CURRENT_BINARY_DIR}/legacygl-gpu-vulkan.txt")
		add_test(NAME legacygl-gpu-record-vulkan
			COMMAND a126cpp-legacygl-gpu-parity-vulkan --record "${A126_GL_GPU_VULKAN_RECORD}"
		)
		set_tests_properties(legacygl-gpu-record-vulkan PROPERTIES
			FIXTURES_SETUP legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-vulkan
			COMMAND a126cpp-legacygl-gpu-parity-native --compare "${A126_GL_GPU_NATIVE_RECORD}" "${A126_GL_GPU_VULKAN_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-vulkan PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-gl33-vulkan
			COMMAND a126cpp-legacygl-gpu-parity-gl33 --compare "${A126_GL_GPU_GL33_RECORD}" "${A126_GL_GPU_VULKAN_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-gl33-vulkan PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-gl21-vulkan
			COMMAND a126cpp-legacygl-gpu-parity-gl21 --compare "${A126_GL_GPU_GL21_RECORD}" "${A126_GL_GPU_VULKAN_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-gl21-vulkan PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)
		list(APPEND A126_GL_GPU_TESTS
			legacygl-gpu-record-vulkan
			legacygl-gpu-compare-vulkan
			legacygl-gpu-compare-gl33-vulkan
			legacygl-gpu-compare-gl21-vulkan
		)
	endif()

	if(A126_ENABLE_D3D12_BACKEND)
		set(A126_GL_GPU_D3D12_RECORD "${CMAKE_CURRENT_BINARY_DIR}/legacygl-gpu-d3d12.txt")
		add_test(NAME legacygl-gpu-record-d3d12
			COMMAND a126cpp-legacygl-gpu-parity-d3d12 --record "${A126_GL_GPU_D3D12_RECORD}"
		)
		set_tests_properties(legacygl-gpu-record-d3d12 PROPERTIES
			FIXTURES_SETUP legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-d3d12
			COMMAND a126cpp-legacygl-gpu-parity-native --compare "${A126_GL_GPU_NATIVE_RECORD}" "${A126_GL_GPU_D3D12_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-d3d12 PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-gl33-d3d12
			COMMAND a126cpp-legacygl-gpu-parity-gl33 --compare "${A126_GL_GPU_GL33_RECORD}" "${A126_GL_GPU_D3D12_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-gl33-d3d12 PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		add_test(NAME legacygl-gpu-compare-gl21-d3d12
			COMMAND a126cpp-legacygl-gpu-parity-gl21 --compare "${A126_GL_GPU_GL21_RECORD}" "${A126_GL_GPU_D3D12_RECORD}"
		)
		set_tests_properties(legacygl-gpu-compare-gl21-d3d12 PROPERTIES
			FIXTURES_REQUIRED legacygl-gpu-records
			RESOURCE_LOCK legacygl-gpu
		)

		list(APPEND A126_GL_GPU_TESTS
			legacygl-gpu-record-d3d12
			legacygl-gpu-compare-d3d12
			legacygl-gpu-compare-gl33-d3d12
			legacygl-gpu-compare-gl21-d3d12
		)

		if(A126_ENABLE_VULKAN_BACKEND)
			add_test(NAME legacygl-gpu-compare-vulkan-d3d12
				COMMAND a126cpp-legacygl-gpu-parity-vulkan --compare "${A126_GL_GPU_VULKAN_RECORD}" "${A126_GL_GPU_D3D12_RECORD}"
			)
			set_tests_properties(legacygl-gpu-compare-vulkan-d3d12 PROPERTIES
				FIXTURES_REQUIRED legacygl-gpu-records
				RESOURCE_LOCK legacygl-gpu
			)
			list(APPEND A126_GL_GPU_TESTS legacygl-gpu-compare-vulkan-d3d12)
		endif()
	endif()

	set_tests_properties(${A126_GL_GPU_TESTS}
		PROPERTIES LABELS "headless;renderer-parity"
	)
endif()
endif()
