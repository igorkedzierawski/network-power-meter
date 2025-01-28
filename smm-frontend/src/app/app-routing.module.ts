import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { MeterComponent } from './newapp/meter/meter.component';

const routes: Routes = [
    { path: 'new', component: MeterComponent, canActivate: [] },
    { path: 'scope', component: MeterComponent, canActivate: [] },
    {
        path: "**",
        redirectTo: "/scope",
        pathMatch: "full"
    }
];

@NgModule({
    imports: [RouterModule.forRoot(routes)],
    exports: [RouterModule]
})
export class AppRoutingModule { }
