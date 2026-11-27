import { Entity } from "../../common/Entity.js";
import { ColorRgb } from "../../common/color/ColorRgb.js";
import type { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
import type { Material } from "./Material.js";
export class SimpleMaterial extends Entity implements Material {
    private readonly name: string;
    private readonly ambient: ColorRgb;
    private readonly diffuse: ColorRgb;
    private readonly specular: ColorRgb;
    private readonly emission: ColorRgb;
    private readonly transmittance: ColorRgb;
    private readonly doubleSided: boolean;
    private readonly reflectionCoefficient: number;
    private readonly refractionCoefficient: number;
    private readonly indexOfRefraction: number;
    private readonly opacity: number;
    private readonly phongExponent: number;
    public constructor();
    public constructor(m: SimpleMaterial);
    public constructor(
        name: string,
        a: ColorRgb,
        d: ColorRgb,
        s: ColorRgb,
        e: ColorRgb,
        t: ColorRgb,
        ds: boolean,
        kr: number,
        kt: number,
        indexOfRefraction: number,
        opacity: number,
        phongExponent: number,
    );
    public constructor(
        name: string,
        a: ColorRgb,
        d: ColorRgb,
        s: ColorRgb,
        ds: boolean,
        kr: number,
        kt: number,
        opacity: number,
        phongExponent: number,
    );
    public constructor(...v: unknown[]) {
        super();
        if (v.length === 0) {
            this.name = "VSDK_default_material";
            this.ambient = new ColorRgb(0.1, 0.1, 0.1);
            this.diffuse = new ColorRgb(0.9, 0.5, 0.5);
            this.specular = new ColorRgb(1, 1, 1);
            this.emission = new ColorRgb();
            this.transmittance = new ColorRgb();
            this.doubleSided = true;
            this.reflectionCoefficient = this.refractionCoefficient = 0;
            this.indexOfRefraction = this.opacity = 1;
            this.phongExponent = 128;
            return;
        }
        if (v[0] instanceof SimpleMaterial) {
            const m = v[0];
            this.name = m.name;
            this.ambient = new ColorRgb(m.ambient);
            this.diffuse = new ColorRgb(m.diffuse);
            this.specular = new ColorRgb(m.specular);
            this.emission = new ColorRgb(m.emission);
            this.transmittance = new ColorRgb(m.transmittance);
            this.doubleSided = m.doubleSided;
            this.reflectionCoefficient = m.reflectionCoefficient;
            this.refractionCoefficient = m.refractionCoefficient;
            this.indexOfRefraction = m.indexOfRefraction;
            this.opacity = m.opacity;
            this.phongExponent = m.phongExponent;
            return;
        }
        const [name, a, d, s, ...r] = v as [string, ColorRgb, ColorRgb, ColorRgb, ...unknown[]];
        this.name = name;
        this.ambient = new ColorRgb(a);
        this.diffuse = new ColorRgb(d);
        this.specular = new ColorRgb(s);
        if (r.length === 5) {
            this.emission = new ColorRgb();
            this.transmittance = new ColorRgb();
            [
                this.doubleSided,
                this.reflectionCoefficient,
                this.refractionCoefficient,
                this.opacity,
                this.phongExponent,
            ] = r as [boolean, number, number, number, number];
            this.indexOfRefraction = 1;
        } else {
            [
                this.emission,
                this.transmittance,
                this.doubleSided,
                this.reflectionCoefficient,
                this.refractionCoefficient,
                this.indexOfRefraction,
                this.opacity,
                this.phongExponent,
            ] = r as [ColorRgb, ColorRgb, boolean, number, number, number, number, number];
            this.emission = new ColorRgb(this.emission);
            this.transmittance = new ColorRgb(this.transmittance);
        }
    }
    public getName(): string {
        return this.name;
    }
    private rebuild(
        change: Partial<{
            name: string;
            a: ColorRgb;
            d: ColorRgb;
            s: ColorRgb;
            e: ColorRgb;
            t: ColorRgb;
            ds: boolean;
            kr: number;
            kt: number;
            ior: number;
            o: number;
            p: number;
        }>,
    ): SimpleMaterial {
        return new SimpleMaterial(
            change.name ?? this.name,
            change.a ?? this.ambient,
            change.d ?? this.diffuse,
            change.s ?? this.specular,
            change.e ?? this.emission,
            change.t ?? this.transmittance,
            change.ds ?? this.doubleSided,
            change.kr ?? this.reflectionCoefficient,
            change.kt ?? this.refractionCoefficient,
            change.ior ?? this.indexOfRefraction,
            change.o ?? this.opacity,
            change.p ?? this.phongExponent,
        );
    }
    public withName(x: string) {
        return this.rebuild({ name: x });
    }
    public withAmbient(x: ColorRgb) {
        return this.rebuild({ a: x });
    }
    public withDiffuse(x: ColorRgb) {
        return this.rebuild({ d: x });
    }
    public withSpecular(x: ColorRgb) {
        return this.rebuild({ s: x });
    }
    public withEmission(x: ColorRgb) {
        return this.rebuild({ e: x });
    }
    public withTransmittance(x: ColorRgb) {
        return this.rebuild({ t: x });
    }
    public withPhongExponent(x: number) {
        return this.rebuild({ p: x });
    }
    public withReflectionCoefficient(x: number) {
        return this.rebuild({ kr: x });
    }
    public withRefractionCoefficient(x: number) {
        return this.rebuild({ kt: x });
    }
    public withIndexOfRefraction(x: number) {
        return this.rebuild({ ior: x });
    }
    public withOpacity(x: number) {
        return this.rebuild({ o: x });
    }
    public withDoubleSided(x: boolean) {
        return this.rebuild({ ds: x });
    }
    public isDoubleSided() {
        return this.doubleSided;
    }
    public copy(): Material {
        return new SimpleMaterial(this);
    }
    public translate(_v: Vector3Dd): Material {
        return this;
    }
    public rotate(_v: Vector3Dd): Material {
        return this;
    }
    public scale(_v: Vector3Dd): Material {
        return this;
    }
    public getAmbient() {
        return new ColorRgb(this.ambient);
    }
    public getAmbientReference() {
        return this.ambient;
    }
    public getDiffuse() {
        return new ColorRgb(this.diffuse);
    }
    public getDiffuseReference() {
        return this.diffuse;
    }
    public getSpecular() {
        return new ColorRgb(this.specular);
    }
    public getSpecularReference() {
        return this.specular;
    }
    public getEmission() {
        return new ColorRgb(this.emission);
    }
    public getEmissionReference() {
        return this.emission;
    }
    public getTransmittance() {
        return new ColorRgb(this.transmittance);
    }
    public getTransmittanceReference() {
        return this.transmittance;
    }
    public getPhongExponent() {
        return this.phongExponent;
    }
    public getReflectionCoefficient() {
        return this.reflectionCoefficient;
    }
    public getRefractionCoefficient() {
        return this.refractionCoefficient;
    }
    public getIndexOfRefraction() {
        return this.indexOfRefraction;
    }
    public getOpacity() {
        return this.opacity;
    }
    public override toString() {
        return `SimpleMaterial [${this.name}]:\n  - Specular ${this.specular}\n  - Diffuse ${this.diffuse}\n  - Ambient ${this.ambient}\n  - Phong exponent: ${this.phongExponent}\n${this.doubleSided ? "  - Double sided" : "  - Single sided"}\n`;
    }
}
