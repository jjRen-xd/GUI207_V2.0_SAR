# -*- coding: utf-8 -*-
import cv2
import torch
import numpy as np
import os
import shutil
import argparse
os.environ["CUDA_VISIBLE_DEVICES"] = "0"

from mmdet.apis import (inference_detector,
                        init_detector, show_result_pyplot)

                    
device = 'cuda'
score_thr = 0.8


class HookValues():
    """
        后期新增，记录中间反传梯度
    """
    def __init__(self, layer, layerName):
        # register a hook to save values of activations and gradients
        self.layerName = layerName
        self.activations = None
        self.gradients = None
        self.forward_hook = layer.register_forward_hook(self.hook_fn_act)
        self.backward_hook = layer.register_backward_hook(self.hook_fn_grad)

    def hook_fn_act(self, module, input, output):
        self.activations = output

    def hook_fn_grad(self, module, grad_input, grad_output):
        self.gradients = grad_output[0]

    def remove(self):
        self.forward_hook.remove()
        self.backward_hook.remove()


def show_feature_map(config, ckpt, img) -> None:
    # build the model from a config file and a checkpoint file
    model = init_detector(config, ckpt, device=device)
    print(model)
    # hook values
    hookValuesMap = {}
    for idx, (name, module) in enumerate(model.named_modules()):
        print(idx, "-", name)
        if isinstance(module, torch.nn.Conv2d):
            hookValuesMap[name] = HookValues(module, name)
    result = inference_detector(model, img)

    
    img_src = cv2.imread(img, 2)
    fourcc = cv2.VideoWriter_fourcc(*"MJPG")
    out_video = cv2.VideoWriter('output.mp4',fourcc, 20.0, (640,480))

    for idx, (name, module) in enumerate(model.named_modules()):
        if isinstance(module, torch.nn.Conv2d):
            print(name, "->", hookValuesMap[name].activations.shape)
            conv_features = torch.sigmoid(hookValuesMap[name].activations)
            conv_features = conv_features.cuda().squeeze(0)

            for i in range(conv_features.shape[0]):
                heatmap = conv_features[i].cpu().numpy()
                heatmap -= heatmap.min()
                heatmap /= heatmap.max()
                heatmap = cv2.resize(heatmap,(img_src.shape[1],img_src.shape[0]))
                heatmap = np.uint8(255*heatmap)

                # 保存视频
                cv2.putText(heatmap, name+"_"+str(i), (10, 20), \
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)
                out_video.write(heatmap)  # 写入帧

                # 保存图像
                # 判断保存的子文件夹是否存在，不存在则创建
                if not os.path.exists('../out_img/'+name):
                    os.makedirs('../out_img/'+name)
                cv2.imwrite("../out_img/"+name+"/"+str(i)+".jpg", heatmap)

                # # 保存卷积核
                # # 判断保存的子文件夹是否存在，不存在则创建
                # if not os.path.exists('../out_img/'+name+"/kernel"):
                #     os.makedirs('../out_img/'+name+"/kernel")
                #     kernal_value = module.weight.data.cpu().numpy()
                #     kernal_value = kernal_value.squeeze(0)
                #     # 将kernal_value转换为图像

                cv2.imshow('superimg',heatmap)
                if cv2.waitKey(20) & 0xFF == ord('q'):  # q退出
                    break

        out_video.release()
        cv2.destroyAllWindows()


def captureActivation(config, ckpt, imgPath, visLayer, savePath = None):
    if os.path.exists(savePath):
        shutil.rmtree(savePath)
    if not os.path.exists(savePath):
        os.makedirs(savePath)

    model = init_detector(config, ckpt, device=device)
    visLayer = f"model.{visLayer}"
    hookValues = HookValues(eval(visLayer), visLayer)
    img_src = cv2.imread(imgPath, 2)

    result = inference_detector(model, imgPath)
    # show_result_pyplot(model, img, result, score_thr=score_thr)
    featureMaps = hookValues.activations[0]
    for i in range(featureMaps.shape[0]):
        featureMap = featureMaps[i].cpu().numpy()
        featureMap -= featureMap.min()
        featureMap /= featureMap.max()
        featureMap = cv2.resize(featureMap,(img_src.shape[1],img_src.shape[0]))
        featureMap = np.uint8(255*featureMap)

        cv2.imwrite(f"{savePath}/{i+1}.png", featureMap)

    print("finished")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='MMDet Visualize a model'
    )
    parser.add_argument(
        '--config', 
        default="/media/z840/HDD_6T/Linux_LIB/mmlab/mmdetection/work_dirs/"+
            "faster_rcnn_r50_fpn_1x_ssdd_20220411/"+
            "faster_rcnn_r50_fpn_1x_ssdd_20220411.py",
        type=str,
        help='test config file path'
    )
    parser.add_argument(
        '--checkpoint', 
        default="/media/z840/HDD_6T/Linux_LIB/mmlab/mmdetection/work_dirs/"+
            "faster_rcnn_r50_fpn_1x_ssdd_20220411/latest.pth",
        type=str,
        help='checkpoint file'
    )
    parser.add_argument(
        '--visualize_layer',
        default="backbone.layer1[0].downsample[0]",
        type=str,
        help='Name of the hidden layer of the model to visualize'
    )
    parser.add_argument(
        '--image_path',
        default="/media/z840/HDD_6T/Linux_LIB/mmlab/mmdetection/datasets/"+
            "SSDD/JPEGImages/000736.jpg",
        type=str,
        help='The path of image to visualize'
    )
    parser.add_argument(
        '--save_path',
        default="/media/z840/HDD_6T/Linux_DATA/jwk/"+
            "GUI207_V2.0_SAR/lib/algorithm/modelVisOutput",
        type=str,
        help='The path of feature map to save'
    )
    args = parser.parse_args()
    captureActivation(args.config, args.checkpoint, args.image_path, \
            args.visualize_layer, args.save_path)