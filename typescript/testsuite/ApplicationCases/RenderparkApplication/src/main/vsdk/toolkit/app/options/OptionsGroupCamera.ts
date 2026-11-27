import { ColorRgbMutable } from "rpk-base/dist/vsdk/toolkit/common/color/ColorRgbMutable";
import { OptionBase } from "rpk-base/dist/vsdk/toolkit/common/commandLineOptions/OptionBase";
import { OptionGroup } from "rpk-base/dist/vsdk/toolkit/common/commandLineOptions/OptionGroup";
import { OptionParser } from "rpk-base/dist/vsdk/toolkit/common/commandLineOptions/OptionParser";
import { TypedOption } from "rpk-base/dist/vsdk/toolkit/common/commandLineOptions/TypedOption";
import { Vector3Dd } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Dd";
import { Camera } from "rpk-base/dist/vsdk/toolkit/scene/Camera";

export class OptionsGroupCamera {
  private static readonly DEFAULT_CAMERA_FIELD_OF_VIEW = 22.5;
  private static readonly DEFAULT_CAMERA_EYE_POSITION = new Vector3Dd(10.0, 0.0, 0.0);
  private static readonly DEFAULT_CAMERA_LOOK_POSITION = new Vector3Dd(0.0, 0.0, 0.0);
  private static readonly DEFAULT_CAMERA_UP_DIRECTION = new Vector3Dd(0.0, 0.0, 1.0);
  private static readonly DEFAULT_BACKGROUND_COLOR = new ColorRgbMutable(0.0, 0.0, 0.0);
  private static cameraState = new Camera();

  private constructor() {
  }

  private static cameraSetEyePositionOption(val: TypedOption.MutableValue<Vector3Dd>): void {
    OptionsGroupCamera.cameraState.setEyePosition(val.value.x(), val.value.y(), val.value.z());
  }

  private static cameraSetLookPositionOption(val: TypedOption.MutableValue<Vector3Dd>): void {
    OptionsGroupCamera.cameraState.setLookPosition(val.value.x(), val.value.y(), val.value.z());
  }

  private static cameraSetUpDirectionOption(val: TypedOption.MutableValue<Vector3Dd>): void {
    OptionsGroupCamera.cameraState.setUpDirection(val.value.x(), val.value.y(), val.value.z());
  }

  private static cameraSetFieldOfViewOption(val: TypedOption.MutableValue<number>): void {
    OptionsGroupCamera.cameraState.setFieldOfView(val.value);
  }

  private static cameraDefaults(camera: Camera, imageWidth: number, imageHeight: number): void {
    const eyePosition = new Vector3Dd(
      OptionsGroupCamera.DEFAULT_CAMERA_EYE_POSITION.x(),
      OptionsGroupCamera.DEFAULT_CAMERA_EYE_POSITION.y(),
      OptionsGroupCamera.DEFAULT_CAMERA_EYE_POSITION.z()
    );
    const lookPosition = new Vector3Dd(
      OptionsGroupCamera.DEFAULT_CAMERA_LOOK_POSITION.x(),
      OptionsGroupCamera.DEFAULT_CAMERA_LOOK_POSITION.y(),
      OptionsGroupCamera.DEFAULT_CAMERA_LOOK_POSITION.z()
    );
    const upDirection = new Vector3Dd(
      OptionsGroupCamera.DEFAULT_CAMERA_UP_DIRECTION.x(),
      OptionsGroupCamera.DEFAULT_CAMERA_UP_DIRECTION.y(),
      OptionsGroupCamera.DEFAULT_CAMERA_UP_DIRECTION.z()
    );
    const backgroundColorSelected = new ColorRgbMutable(
      OptionsGroupCamera.DEFAULT_BACKGROUND_COLOR.r,
      OptionsGroupCamera.DEFAULT_BACKGROUND_COLOR.g,
      OptionsGroupCamera.DEFAULT_BACKGROUND_COLOR.b
    );

    camera.set(
      eyePosition,
      lookPosition,
      upDirection,
      OptionsGroupCamera.DEFAULT_CAMERA_FIELD_OF_VIEW,
      imageWidth,
      imageHeight,
      backgroundColorSelected
    );
  }

  private static parseVector3(
    argc: number,
    argv: string[] | null,
    value: TypedOption.MutableValue<Vector3Dd>
  ): boolean {
    if (argc < 3 || argv === null || argv[0] === null || argv[1] === null || argv[2] === null) {
      return false;
    }
    const x = Number.parseFloat(argv[0]!);
    const y = Number.parseFloat(argv[1]!);
    const z = Number.parseFloat(argv[2]!);
    if (Number.isNaN(x) || Number.isNaN(y) || Number.isNaN(z)) {
      return false;
    }
    value.value = value.value.withX(x);
    value.value = value.value.withY(y);
    value.value = value.value.withZ(z);
    return true;
  }

  public static cameraParseOptions(
    argc: number[],
    argv: string[],
    camera: Camera,
    imageWidth: number,
    imageHeight: number
  ): void {
    const eyePointOpt = new TypedOption<Vector3Dd>(
      "-eyepoint",
      TypedOption.reference(
        () => OptionsGroupCamera.cameraState.eyePosition,
        (v) => {
          OptionsGroupCamera.cameraState.eyePosition = v;
        }
      ),
      3,
      OptionsGroupCamera.cameraSetEyePositionOption,
      OptionsGroupCamera.parseVector3
    );
    const centerOpt = new TypedOption<Vector3Dd>(
      "-center",
      TypedOption.reference(
        () => OptionsGroupCamera.cameraState.lookPosition,
        (v) => {
          OptionsGroupCamera.cameraState.lookPosition = v;
        }
      ),
      3,
      OptionsGroupCamera.cameraSetLookPositionOption,
      OptionsGroupCamera.parseVector3
    );
    const upDirOpt = new TypedOption<Vector3Dd>(
      "-updir",
      TypedOption.reference(
        () => OptionsGroupCamera.cameraState.upDirection,
        (v) => {
          OptionsGroupCamera.cameraState.upDirection = v;
        }
      ),
      3,
      OptionsGroupCamera.cameraSetUpDirectionOption,
      OptionsGroupCamera.parseVector3
    );
    const fovOpt = new TypedOption<number>(
      "-fov",
      TypedOption.reference(
        () => OptionsGroupCamera.cameraState.fieldOfVision,
        (v) => {
          OptionsGroupCamera.cameraState.fieldOfVision = v;
        }
      ),
      1,
      OptionsGroupCamera.cameraSetFieldOfViewOption,
      null
    );
    const cameraOptions: OptionBase[] = [
      TypedOption.REGISTER_OPTION(eyePointOpt, 4),
      TypedOption.REGISTER_OPTION(centerOpt, 4),
      TypedOption.REGISTER_OPTION(upDirOpt, 3),
      TypedOption.REGISTER_OPTION(fovOpt, 4),
    ];

    OptionsGroupCamera.cameraDefaults(OptionsGroupCamera.cameraState, imageWidth, imageHeight);
    const cameraGroups = [
      new OptionGroup("camera", cameraOptions, 4),
    ];
    OptionParser.parse(argc, argv, cameraGroups, 1);
    camera.set(
      OptionsGroupCamera.cameraState.eyePosition,
      OptionsGroupCamera.cameraState.lookPosition,
      OptionsGroupCamera.cameraState.upDirection,
      OptionsGroupCamera.cameraState.fieldOfVision,
      OptionsGroupCamera.cameraState.xSize,
      OptionsGroupCamera.cameraState.ySize,
      OptionsGroupCamera.cameraState.background
    );
  }
}
