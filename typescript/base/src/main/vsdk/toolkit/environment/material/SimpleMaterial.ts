import { Entity } from "../../common/Entity.js";
import { ColorRgb } from "../../common/color/ColorRgb.js";
import type { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
import { Double } from "../../../../java/lang/Double.js";
import type { Material } from "./Material.js";

export class SimpleMaterial extends Entity implements Material {
    private readonly ambient: ColorRgb;
    private readonly diffuse: ColorRgb;
    private readonly specular: ColorRgb;
    private readonly emission: ColorRgb;
    private readonly transmittance: ColorRgb;
    private readonly doubleSided: boolean;
    private readonly reflectionCoefficient: number;
    private readonly refractionCoefficient: number; // Also known as "transmission"
    private readonly indexOfRefraction: number;
    private readonly name: string;
    private readonly opacity: number;
    private readonly phongExponent: number;

    /** Creates a new instance of MaterialGL */
    public constructor();
    public constructor(m: SimpleMaterial);
    public constructor(
        name: string,
        ambient: ColorRgb,
        diffuse: ColorRgb,
        specular: ColorRgb,
        emission: ColorRgb,
        transmittance: ColorRgb,
        doubleSided: boolean,
        reflectionCoefficient: number,
        refractionCoefficient: number,
        indexOfRefraction: number,
        opacity: number,
        phongExponent: number,
    );
    public constructor(
        name: string,
        ambient: ColorRgb,
        diffuse: ColorRgb,
        specular: ColorRgb,
        doubleSided: boolean,
        reflectionCoefficient: number,
        refractionCoefficient: number,
        opacity: number,
        phongExponent: number,
    );
    /**
    Java's four constructors chain into the twelve-argument one; here the
    overload is chosen by the argument count and its values flow into that
    same body.
    */
    public constructor(...v: unknown[]) {
        super();
        let name: string;
        let ambient: ColorRgb;
        let diffuse: ColorRgb;
        let specular: ColorRgb;
        let emission: ColorRgb;
        let transmittance: ColorRgb;
        let doubleSided: boolean;
        let reflectionCoefficient: number;
        let refractionCoefficient: number;
        let indexOfRefraction: number;
        let opacity: number;
        let phongExponent: number;
        if (v.length === 0) {
            name = "VSDK_default_material";
            ambient = new ColorRgb(0.1, 0.1, 0.1);
            diffuse = new ColorRgb(0.9, 0.5, 0.5);
            specular = new ColorRgb(1, 1, 1);
            emission = new ColorRgb(0, 0, 0);
            transmittance = new ColorRgb(0, 0, 0);
            doubleSided = true;
            reflectionCoefficient = 0.0;
            refractionCoefficient = 0.0;
            indexOfRefraction = 1.0;
            opacity = 1.0;
            phongExponent = 128.0;
        } else if (v.length === 1) {
            const m = v[0] as SimpleMaterial;
            name = m.name;
            ambient = m.ambient;
            diffuse = m.diffuse;
            specular = m.specular;
            emission = m.emission;
            transmittance = m.transmittance;
            doubleSided = m.doubleSided;
            reflectionCoefficient = m.reflectionCoefficient;
            refractionCoefficient = m.refractionCoefficient;
            indexOfRefraction = m.indexOfRefraction;
            opacity = m.opacity;
            phongExponent = m.phongExponent;
        } else if (v.length === 9) {
            [
                name,
                ambient,
                diffuse,
                specular,
                doubleSided,
                reflectionCoefficient,
                refractionCoefficient,
                opacity,
                phongExponent,
            ] = v as [string, ColorRgb, ColorRgb, ColorRgb, boolean, number, number, number, number];
            emission = new ColorRgb(0, 0, 0);
            transmittance = new ColorRgb(0, 0, 0);
            indexOfRefraction = 1.0;
        } else {
            [
                name,
                ambient,
                diffuse,
                specular,
                emission,
                transmittance,
                doubleSided,
                reflectionCoefficient,
                refractionCoefficient,
                indexOfRefraction,
                opacity,
                phongExponent,
            ] = v as [
                string,
                ColorRgb,
                ColorRgb,
                ColorRgb,
                ColorRgb,
                ColorRgb,
                boolean,
                number,
                number,
                number,
                number,
                number,
            ];
        }
        this.name = name;
        this.ambient = new ColorRgb(ambient);
        this.diffuse = new ColorRgb(diffuse);
        this.specular = new ColorRgb(specular);
        this.emission = new ColorRgb(emission);
        this.transmittance = new ColorRgb(transmittance);
        this.doubleSided = doubleSided;
        this.reflectionCoefficient = reflectionCoefficient;
        this.refractionCoefficient = refractionCoefficient;
        this.indexOfRefraction = indexOfRefraction;
        this.opacity = opacity;
        this.phongExponent = phongExponent;
    }

    public getName(): string {
        return this.name;
    }

    public withName(n: string): SimpleMaterial {
        return new SimpleMaterial(
            n,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withAmbient(a: ColorRgb): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            a,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withDiffuse(d: ColorRgb): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            d,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withSpecular(s: ColorRgb): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            s,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withEmission(e: ColorRgb): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            e,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withTransmittance(t: ColorRgb): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            t,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withPhongExponent(p: number): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            p,
        );
    }

    public withReflectionCoefficient(kr: number): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            kr,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withRefractionCoefficient(kr: number): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            kr,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public withIndexOfRefraction(ior: number): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            ior,
            this.opacity,
            this.phongExponent,
        );
    }

    public withOpacity(a: number): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            this.doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            a,
            this.phongExponent,
        );
    }

    public isDoubleSided(): boolean {
        return this.doubleSided;
    }

    public copy(): Material {
        return new SimpleMaterial(this);
    }

    public translate(_vector: Vector3Dd): Material {
        return this;
    }

    public rotate(_vector: Vector3Dd): Material {
        return this;
    }

    public scale(_vector: Vector3Dd): Material {
        return this;
    }

    public withDoubleSided(doubleSided: boolean): SimpleMaterial {
        return new SimpleMaterial(
            this.name,
            this.ambient,
            this.diffuse,
            this.specular,
            this.emission,
            this.transmittance,
            doubleSided,
            this.reflectionCoefficient,
            this.refractionCoefficient,
            this.indexOfRefraction,
            this.opacity,
            this.phongExponent,
        );
    }

    public getAmbient(): ColorRgb {
        return new ColorRgb(this.ambient);
    }

    public getAmbientReference(): ColorRgb {
        return this.ambient;
    }

    public getDiffuse(): ColorRgb {
        return new ColorRgb(this.diffuse);
    }

    public getDiffuseReference(): ColorRgb {
        return this.diffuse;
    }

    public getSpecular(): ColorRgb {
        return new ColorRgb(this.specular);
    }

    public getSpecularReference(): ColorRgb {
        return this.specular;
    }

    public getEmission(): ColorRgb {
        return new ColorRgb(this.emission);
    }

    public getEmissionReference(): ColorRgb {
        return this.emission;
    }

    public getTransmittance(): ColorRgb {
        return new ColorRgb(this.transmittance);
    }

    public getTransmittanceReference(): ColorRgb {
        return this.transmittance;
    }

    public getPhongExponent(): number {
        return this.phongExponent;
    }

    public getReflectionCoefficient(): number {
        return this.reflectionCoefficient;
    }

    public getRefractionCoefficient(): number {
        return this.refractionCoefficient;
    }

    public getIndexOfRefraction(): number {
        return this.indexOfRefraction;
    }

    public getOpacity(): number {
        return this.opacity;
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current SimpleMaterial information
    */
    public override toString(): string {
        return (
            "SimpleMaterial [" +
            this.name +
            "]:\n" +
            "  - Specular " +
            this.specular.toString() +
            "\n" +
            "  - Diffuse " +
            this.diffuse.toString() +
            "\n" +
            "  - Ambient " +
            this.ambient.toString() +
            "\n" +
            "  - Phong exponent: " +
            Double.toString(this.phongExponent) +
            "\n" +
            (this.isDoubleSided() ? "  - Double sided\n" : "  - Single sided\n") +
            "\n"
        );
    }
}
