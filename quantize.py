import sys
import os
import onnx
from onnxruntime.quantization import quantize_dynamic, QuantType
from ultralytics import YOLO

def main():
    # 1. 接收 C++ 程序传入的参数
    # python quantize.py <输入的.pt模型路径> <输出目录> <量化精度>
    if len(sys.argv) < 4:
        print("Usage: python quantize.py <input_model> <output_dir> <precision>")
        return

    input_model = sys.argv[1]      # PyTorch模型路径 (.pt)
    output_dir = sys.argv[2]       # 输出目录
    precision = sys.argv[3]        # 量化精度 ("INT8 (推荐)", "FP16", "INT4")

    # 2. 生成输出文件名
    base_name = os.path.splitext(os.path.basename(input_model))[0]
    fp32_onnx_path = os.path.join(output_dir, f"{base_name}_fp32.onnx")
    int8_onnx_path = os.path.join(output_dir, f"{base_name}_int8.onnx")

    try:
        # ========== 第一步：导出普通 FP32 的 ONNX 模型 ==========
        print(f"正在导出 FP32 ONNX 模型: {fp32_onnx_path}")
        model = YOLO(input_model)
        model.export(format="onnx", imgsz=640)       # 注意：这里不指定 out 路径
        print("ONNX 导出完成")

        # 手动找到并重命名导出的 ONNX 文件
        default_onnx_path = f"{input_model.rsplit('.', 1)[0]}.onnx"
        if os.path.exists(default_onnx_path):
            os.rename(default_onnx_path, fp32_onnx_path)
            print(f"ONNX 模型已保存至: {fp32_onnx_path}")
        else:
            raise FileNotFoundError("未能找到导出的 ONNX 模型")

        # ========== 第二步：进行 INT8 动态量化 ==========
        # 检查用户选择的精度，如果不是 INT8，就不做量化，直接使用 FP32 模型
        if "INT8" in precision.upper():
            print(f"正在对 ONNX 模型进行 INT8 动态量化: {int8_onnx_path}")
            quantize_dynamic(fp32_onnx_path, int8_onnx_path, weight_type=QuantType.QUInt8)
            print("INT8 量化完成")
            final_model_path = int8_onnx_path
        else:
            print("FP16 或 INT4 量化暂不支持，直接使用 FP32 的 ONNX 模型")
            final_model_path = fp32_onnx_path

        # ========== 第三步：确认模型文件存在，并通知 C++ ==========
        if os.path.exists(final_model_path):
            # 这一行是关键！C++ 程序会解析这个输出来获取模型路径
            print(f"Success: {final_model_path}")
        else:
            print(f"Error: 模型文件未找到 - {final_model_path}")

    except Exception as e:
        print(f"Error: {str(e)}")

if __name__ == "__main__":
    main()