import bpy

bl_info = {
    "name": "NDS Model Lighting Baker",
    "author": "P3D Team",
    "version": (1, 0, 0),
    "blender": (4, 0, 0),
    "location": "View3D > Sidebar > NDS Engine",
    "description": "Bakes Vertex Lighting into Vertex Colors (COLOR_0) for gltf export & NDS hardware.",
    "category": "3D View",
}


class OBJECT_OT_bake_vertex_lighting(bpy.types.Operator):
    bl_idname = "object.bake_vertex_lighting"
    bl_label = "Bake Vertex Lighting"
    bl_description = "Bakes lighting into COLOR_0 vertex colors for gltf export. This is a destructive operation, so make sure to save your work before proceeding."
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        # Target selected mesh objects
        selected_objects = [
            obj for obj in context.selected_objects if obj.type == "MESH"
        ]
        if not selected_objects:
            self.report({"WARNING"}, "No mesh objects selected.")
            return {"CANCELLED"}

        scene = context.scene

        # Configure Cycles settings for baking
        orig_engine = scene.render.engine
        scene.render.engine = "CYCLES"
        scene.cycles.samples = 64
        scene.cycles.bake_type = "DIFFUSE"
        scene.render.bake.use_pass_direct = True
        scene.render.bake.use_pass_indirect = True
        scene.render.bake.use_pass_color = False
        scene.render.bake.target = "VERTEX_COLORS"

        for obj in selected_objects:
            context.view_layer.objects.active = obj

            # Create or select COLOR_0 attribute
            if "COLOR_0" not in obj.data.color_attributes:
                obj.data.color_attributes.new(
                    name="COLOR_0", type="FLOAT_COLOR", domain="CORNER"
                )
            obj.data.color_attributes.active_color = obj.data.color_attributes[
                "COLOR_0"
            ]

            # Clean transforms & custom normals
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            if hasattr(obj.data, "clear_custom_normals"):
                try:
                    obj.data.clear_custom_normals()
                except Exception:
                    pass

        # Excecute the bake operation
        bpy.ops.object.select_all(action="DESELECT")
        for obj in selected_objects:
            obj.select_set(True)

        context.view_layer.objects.active = selected_objects[0]
        bpy.ops.object.bake(type="DIFFUSE")

        scene.render.engine = orig_engine  # Restore original render engine

        # Connect COLOR_0 using version compatible node fallback
        for obj in selected_objects:
            for slot in obj.material_slots:
                mat = slot.material
                if not mat or not mat.use_nodes:
                    continue

                nodes = mat.node_tree.nodes
                links = mat.node_tree.links
                bsdf = next((n for n in nodes if n.type == "BSDF_PRINCIPLED"), None)
                if not bsdf:
                    continue

                # Get existing vertex color node or create one
                vc_node = nodes.get("GLTF_COLOR_0")
                if not vc_node:
                    try:
                        vc_node = nodes.new("ShaderNodeColorAttribute")
                    except Exception:
                        try:
                            vc_node = nodes.new("ShaderNodeVertexColor")
                        except Exception:
                            vc_node = nodes.new("ShaderNodeAttribute")

                    vc_node.name = "GLTF_COLOR_0"

                # Assign attribute name based on node API properties
                if hasattr(vc_node, "layer_name"):
                    vc_node.layer_name = "COLOR_0"
                elif hasattr(vc_node, "attribute_name"):
                    vc_node.attribute_name = "COLOR_0"

                vc_node.location = (bsdf.location.x - 500, bsdf.location.y - 150)
                base_color = bsdf.inputs["Base Color"]

                if base_color.is_linked:
                    from_socket = base_color.links[0].from_socket
                    if from_socket.node.name != "LightmapMix":
                        mix = nodes.new("ShaderNodeMix")
                        mix.name = "LightmapMix"
                        mix.data_type = "RGBA"
                        mix.blend_type = "MULTIPLY"
                        mix.inputs["Factor"].default_value = 1.0
                        mix.location = (bsdf.location.x - 250, bsdf.location.y)

                        links.new(from_socket, mix.inputs["A"])
                        links.new(vc_node.outputs["Color"], mix.inputs["B"])
                        links.new(mix.outputs["Result"], base_color)

                else:
                    links.new(vc_node.outputs["Color"], base_color)

        self.report(
            {"INFO"},
            f"Successfuly baked vertex lighting on {len(selected_objects)} object(s)!",
        )
        return {"FINISHED"}


class VIEW3D_PT_nds_lighting_baker(bpy.types.Panel):
    bl_label = "NDS Model Lighting Baker"
    bl_idname = "VIEW3D_PT_nds_lighting_baker"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "NDS Engine"

    def draw(self, context):
        layout = self.layout
        layout.label(text="Vertex Color Baking")
        layout.operator(OBJECT_OT_bake_vertex_lighting.bl_idname, icon="GROUP_VCOL")


classes = (
    OBJECT_OT_bake_vertex_lighting,
    VIEW3D_PT_nds_lighting_baker,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
