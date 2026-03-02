import sys
import numpy as np

def analyze_obj_centroid(obj_file_path):
    """
    分析OBJ文件的顶点，计算平均值来判断模型原点是否在几何中心
    """
    vertices = []
    
    try:
        with open(obj_file_path, 'r') as f:
            lines = f.readlines()
            
        for line in lines:
            line = line.strip()
            if line.startswith('v ') and not line.startswith('vt ') and not line.startswith('vn '):
                # 顶点坐标行: v x y z
                parts = line.split()
                if len(parts) >= 4:
                    try:
                        x = float(parts[1])
                        y = float(parts[2])
                        z = float(parts[3])
                        vertices.append([x, y, z])
                    except ValueError:
                        continue
        
        if not vertices:
            print(f"错误: {obj_file_path} 中没有找到顶点数据")
            return None
            
        vertices_array = np.array(vertices)
        
        # 计算统计信息
        min_coords = vertices_array.min(axis=0)
        max_coords = vertices_array.max(axis=0)
        avg_coords = vertices_array.mean(axis=0)
        centroid = avg_coords  # 质心
        
        print("=" * 60)
        print(f"OBJ文件分析: {obj_file_path}")
        print(f"顶点数量: {len(vertices)}")
        print("=" * 60)
        print("坐标范围:")
        print(f"  X: [{min_coords[0]:.6f}, {max_coords[0]:.6f}]")
        print(f"  Y: [{min_coords[1]:.6f}, {max_coords[1]:.6f}]")
        print(f"  Z: [{min_coords[2]:.6f}, {max_coords[2]:.6f}]")
        print()
        print(f"质心/平均位置: ({centroid[0]:.6f}, {centroid[1]:.6f}, {centroid[2]:.6f})")
        print()
        
        # 计算边界框中心
        bbox_center = (min_coords + max_coords) / 2
        print(f"边界框中心:   ({bbox_center[0]:.6f}, {bbox_center[1]:.6f}, {bbox_center[2]:.6f})")
        print()
        
        # 判断原点是否在中心
        centroid_distance = np.linalg.norm(centroid)
        bbox_center_distance = np.linalg.norm(bbox_center)
        
        print(f"质心到原点的距离: {centroid_distance:.6f}")
        print(f"边界框中心到原点的距离: {bbox_center_distance:.6f}")
        print()
        
        # 阈值判断
        threshold = 0.01  # 可以调整这个阈值
        
        if centroid_distance < threshold:
            print("✓ 质心接近原点 (在几何中心)")
        else:
            print("✗ 质心偏离原点")
            print(f"  偏移方向: ", end="")
            if abs(centroid[0]) > threshold/10: print(f"X:{centroid[0]:+.3f} ", end="")
            if abs(centroid[1]) > threshold/10: print(f"Y:{centroid[1]:+.3f} ", end="")
            if abs(centroid[2]) > threshold/10: print(f"Z:{centroid[2]:+.3f} ", end="")
            print()
        
        if bbox_center_distance < threshold:
            print("✓ 边界框中心接近原点")
        else:
            print("✗ 边界框中心偏离原点")
        
        # 计算模型大小
        model_size = max_coords - min_coords
        max_dimension = max(model_size)
        print()
        print(f"模型尺寸: ({model_size[0]:.3f}, {model_size[1]:.3f}, {model_size[2]:.3f})")
        print(f"最大维度: {max_dimension:.3f}")
        
        return {
            'vertices_count': len(vertices),
            'min_coords': min_coords,
            'max_coords': max_coords,
            'centroid': centroid,
            'bbox_center': bbox_center,
            'centroid_distance': centroid_distance,
            'bbox_center_distance': bbox_center_distance,
            'model_size': model_size,
            'max_dimension': max_dimension
        }
        
    except FileNotFoundError:
        print(f"错误: 找不到文件 {obj_file_path}")
        return None
    except Exception as e:
        print(f"分析文件时出错: {e}")
        return None

def normalize_obj_to_origin(obj_file_path, output_file_path=None):
    """
    将OBJ文件的顶点平移到原点（质心移动到原点）
    并可选地保存为新文件
    """
    vertices = []
    vertex_lines = []
    other_lines = []
    
    try:
        with open(obj_file_path, 'r') as f:
            lines = f.readlines()
            
        # 分离顶点和其他数据
        vertex_indices = []
        for i, line in enumerate(lines):
            line_stripped = line.strip()
            if line_stripped.startswith('v ') and not line_stripped.startswith('vt ') and not line_stripped.startswith('vn '):
                parts = line_stripped.split()
                if len(parts) >= 4:
                    try:
                        x = float(parts[1])
                        y = float(parts[2])
                        z = float(parts[3])
                        vertices.append([x, y, z])
                        vertex_lines.append(line)
                        vertex_indices.append(i)
                    except ValueError:
                        other_lines.append((i, line))
            else:
                other_lines.append((i, line))
        
        if not vertices:
            print("没有找到顶点数据")
            return False
        
        vertices_array = np.array(vertices)
        centroid = vertices_array.mean(axis=0)
        
        print(f"\n原始质心: ({centroid[0]:.6f}, {centroid[1]:.6f}, {centroid[2]:.6f})")
        print("正在将模型平移到原点...")
        
        # 平移所有顶点
        normalized_vertices = vertices_array - centroid
        
        # 如果指定了输出文件，则保存
        if output_file_path:
            # 重新构建文件内容
            new_lines = []
            vertex_idx = 0
            
            for i, line in other_lines:
                # 插入顶点行
                while vertex_idx < len(vertex_indices) and vertex_indices[vertex_idx] < i:
                    v = normalized_vertices[vertex_idx]
                    new_lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
                    vertex_idx += 1
                new_lines.append(line)
            
            # 处理剩余的顶点行
            while vertex_idx < len(vertex_indices):
                v = normalized_vertices[vertex_idx]
                new_lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
                vertex_idx += 1
            
            # 写入新文件
            with open(output_file_path, 'w') as f:
                f.writelines(new_lines)
            
            print(f"已保存归一化后的模型到: {output_file_path}")
            
            # 验证新文件
            print("\n验证归一化后的模型:")
            new_stats = analyze_obj_centroid(output_file_path)
            
        return True
        
    except Exception as e:
        print(f"归一化过程中出错: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("使用方法:")
        print(f"  python {sys.argv[0]} <obj文件路径> [输出文件路径]")
        print("\n示例:")
        print(f"  python {sys.argv[0]} sphere.obj")
        print(f"  python {sys.argv[0]} sphere.obj sphere_normalized.obj")
        sys.exit(1)
    
    obj_path = sys.argv[1]
    
    # 分析原始文件
    stats = analyze_obj_centroid(obj_path)
    
    # 如果需要归一化
    if len(sys.argv) >= 3 and stats is not None:
        output_path = sys.argv[2]
        if stats['centroid_distance'] > 0.01:
            print("\n" + "="*60)
            print("尝试将模型归一化到原点...")
            normalize_obj_to_origin(obj_path, output_path)
        else:
            print("\n模型已经在原点附近，无需归一化。")