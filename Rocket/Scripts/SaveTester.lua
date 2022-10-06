SaveTester =
{
    numJumps = 0,
    numBoosts = 0
}

function SaveTester:Tick(deltaTime)

    if (Input.IsGamepadButtonJustDown(Gamepad.Up)) then
        self:Load()
    end

    if (Input.IsGamepadButtonJustDown(Gamepad.Down)) then
        self:Save()
    end

    if (Input.IsGamepadButtonJustDown(Gamepad.A)) then
        self.numJumps = self.numJumps + 1
        Log.Debug("JUMPS = " .. self.numJumps)
    end

    if (Input.IsGamepadButtonJustDown(Gamepad.B)) then
        self.numBoosts = self.numBoosts + 1
        Log.Debug("BOOSTS = " .. self.numBoosts)
    end
end

function SaveTester:Save()

    local car = world:FindActor("Car")

    local stream = Stream.Create()
    stream:WriteInt32(self.numJumps)
    stream:WriteInt8(self.numBoosts)
    stream:WriteString("Yeee Dolpin!")
    stream:WriteVec3(car:GetPosition())

    System.WriteSave("RocketLua.sav", stream)

end


function SaveTester:Load()

    local car = world:FindActor("Car")

    if System.DoesSaveExist("RocketLua.sav") then
        local car = world:FindActor("Car")
        local stream = Stream.Create()
        System.ReadSave("RocketLua.sav", stream)

        self.numJumps = stream:ReadInt32()
        self.numBoosts = stream:ReadInt8()
        local testString = stream:ReadString()
        local carPos = stream:ReadVec3()

        Log.Debug("numJumps = " .. self.numJumps)
        Log.Debug("numBoosts = " .. self.numBoosts)
        Log.Debug(testString)
        car:SetPosition(carPos)
    end
end