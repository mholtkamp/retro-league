
function dump(o)
    if type(o) == 'table' then
       local s = '{ '
       for k,v in pairs(o) do
          if type(k) ~= 'number' then k = '"'..k..'"' end
          s = s .. '['..k..'] = ' .. dump(v) .. ','
       end
       return s .. '} '
    else
       return tostring(o)
    end
 end


function dumpKeys(o)
    if type(o) == 'table' then
        local s = '{ '
        for k in pairs(o) do
            s = s .. ' ' .. k .. ' '
        end
        return s .. '} '
     else
        return tostring(o)
     end
end

TestRocket = { time = 0, yoffset = Vector.Create(0, 0, 0), timeScale = 1.0 }

function TestRocket:Create(deltaTime)
   local foundActor = world:FindActor("TestActor")

   if (foundActor) then
      local foundComp = foundActor:GetComponent("Right Mesh 1")

      --foundActor:SetName("Beepzzz")
      foundActor:SetScale(Vector.Create(0.8, 0.8, 0.8))


      local grassMat = LoadAsset("M_ArenaM2_Grass")

      if (grassMat) then
         self.grassInst = MaterialInstance.Create(grassMat)
         self.grassInst:SetColor(Vec(0.2, 0.3, 0.9, 1.0))
         foundComp:SetMaterialOverride(self.grassInst)
      else
         Engine.Log("Could not find grass material??")
      end
   end

end

function TestRocket:Create()
   self.aRef = ActorRef.Create()
   self.cRef = ComponentRef.Create()
end

function TestRocket:Tick(deltaTime)

   local rootComp = self.actor:GetRootComponent()

   self.time = self.time + deltaTime
   local newPos = Vector.Create()
   local rotAxis = Vector.Create()
   rotAxis.x = 0.0
   rotAxis.y = 1.0
   rotAxis.z = 0.0

   newPos.x = math.sin(self.time * self.timeScale) * 5.0
   newPos = newPos:Rotate(self.time, rotAxis)
   --newPos = Vector.Rotate(newPos, self.time * 4, rotAxis)
   --newPos = newPos.Rotate(newPos, self.time * 4, rotAxis)

   local goHigh = ((self.time * self.timeScale) % 2) >= 1
   local yTarget = goHigh and 5.0 or 0.0 
   local yTargetVec = Vector.Create(0, yTarget, 0)
   self.yoffset = Vector.Damp(self.yoffset, yTargetVec, 0.005, deltaTime)

   if (Input.IsKeyDown(Key.N1) or Input.IsGamepadButtonDown(Gamepad.A, 2)) then
      local deltaScale = Vector.Create(2.0, 2.0, 2.0)
      local newScale = rootComp:GetScale() + deltaScale * deltaTime
      newScale = newScale:Clamp(Vector.Create(0.1, 0.1, 0.1), Vector.Create(5.0, 5.0, 5.0))
      rootComp:SetScale(newScale)
   elseif (Input.IsKeyDown(Key.Two) or Input.IsGamepadButtonDown(Gamepad.B)) then
      local deltaScale = Vector.Create(-2.0, -2.0, -2.0)
      local newScale = rootComp:GetScale() + deltaScale * deltaTime
      newScale = newScale:Clamp(Vector.Create(0.1, 0.1, 0.1), Vector.Create(5.0, 5.0, 5.0))
      rootComp:SetScale(newScale)
   end


   if (Input.IsKeyJustDown(Key.F1)) then
      self.aRef:Set(world:FindActor(self.searchString))
   end

   if (Input.IsKeyJustDown(Key.F2)) then
      if (self.aRef:Get()) then
         Log.Debug('aRef is ' .. self.aRef:Get():GetName())
      else
         Log.Debug('aRef is null')
      end
   end

   if (Input.IsKeyJustDown(Key.F3)) then
      local actor = world:FindActor(self.searchString)
      if (actor) then
         self.cRef:Set(actor:GetRootComponent())
      end
   end

   if (Input.IsKeyJustDown(Key.F4)) then
      if (self.cRef:Get()) then
         Log.Debug('cRef is ' .. self.cRef:Get():GetName())
      else
         Log.Debug('cRef is null')
      end
   end

   newPos = newPos + self.yoffset
   rootComp:SetPosition(newPos)



   --[[
   if (Input.IsKeyDown(Key.F1)) then
      local testSphere = Find
   end
   ]]--

end

function TestRocket:GatherProperties()
   return
   {
      { name = "timeScale", type = DatumType.Float },
      { name = "mesh", type = DatumType.Asset },
      { name = "searchString", type = DatumType.String }
   }
end

